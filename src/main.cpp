/**
 * @file src/main.cpp
 * @brief Definitions for the main entry point for Sunshine.
 */
// standard includes
#include <charconv>
#include <codecvt>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>


// local includes
#include "entry_handler.h"
#include "globals.h"
#include "httpcommon.h"
#include "logging.h"
#include "main.h"
#include "nvhttp.h"
#include "process.h"
#include "session_stream.h"
#ifdef __linux__
  #include "session/session_context.h"
#endif
#include "video.h"

#ifdef PLANK_TRANSPORT
  #include <plank_transport.h>
#endif

using namespace std::literals;

std::map<int, std::function<void()>> signal_handlers;  ///< Signal handlers.

/**
 * @brief Forward a POSIX signal to the registered Sunshine handler.
 *
 * @param sig Native signal number being handled.
 */
void on_signal_forwarder(int sig) {
  signal_handlers.at(sig)();
}

/**
 * @brief Register the handler invoked for a POSIX signal.
 *
 * @param sig Native signal number being handled.
 * @param fn Signal handler function to install.
 */
template<class FN>
void on_signal(int sig, FN &&fn) {
  signal_handlers.emplace(sig, std::forward<FN>(fn));

  std::signal(sig, on_signal_forwarder);
}

/**
 * @brief Cmd to func.
 */
std::map<std::string_view, std::function<int(const char *name, int argc, char **argv)>> cmd_to_func {
  {"help"sv, [](const char *name, int argc, char **argv) {
     return args::help(name);
   }},
  {"version"sv, [](const char *name, int argc, char **argv) {
     return args::version();
   }},
#ifdef _WIN32
  {"restore-nvprefs-undo"sv, [](const char *name, int argc, char **argv) {
     return args::restore_nvprefs_undo();
   }},
#endif
#ifdef __linux__
  // Root-only hardware qualification: capture a display arrangement through
  // the stream's packed-capture path without a client. See the host's
  // systemd README, "Qualify a workstation's display arrangements".
  {"capture-probe"sv, [](const char *name, int argc, char **argv) {
     if (argc < 2 || argc > 4) {
       std::cerr << "usage: " << name << " [CONFIG] --capture-probe REQUEST ENCODING_MODE [FRAMES] [OUTPUT_DIRECTORY]\n";
       return 2;
     }
     int frames = 120;
     if (argc >= 3) {
       const std::string_view value {argv[2]};
       const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), frames);
       if (error != std::errc {} || end != value.data() + value.size()) {
         std::cerr << "FRAMES must be a number\n";
         return 2;
       }
     }
     const std::filesystem::path output {argc >= 4 ? argv[3] : "/var/tmp/plank-capture-probe"};
     if (!plank::arrangement::parse(argv[0]).request || frames < 1 || frames > 3600 ||
         !output.is_absolute()) {
       std::cerr << "REQUEST must be a canonical display arrangement, FRAMES 1 to 3600, "
                    "and OUTPUT_DIRECTORY absolute\n";
       return 2;
     }
     auto platform = platf::init();
     if (!platform) {
       std::cerr << "PLANK platform initialization failed\n";
       return 3;
     }
     return video::capture_probe(argv[0], argv[1], frames, output);
   }},
#endif
};


/**
 * @brief Run the main event loop until Sunshine is asked to exit.
 *
 * @param shutdown_event Shutdown event.
 */
void mainThreadLoop(const std::shared_ptr<safe::event_t<bool>> &shutdown_event) {
  shutdown_event->view();
}

/**
 * @brief Run the main application or worker loop.
 *
 * @param argc The number of arguments.
 * @param argv The arguments.
 * @return Process or platform callback exit code.
 */
int main(int argc, char *argv[]) {

  lifetime::argv = argv;

  task_pool_util::TaskPool::task_id_t force_shutdown = nullptr;



  mail::man = std::make_shared<safe::mail_raw_t>();

  // parse config file
  if (config::parse(argc, argv)) {
    return 0;
  }

  auto log_deinit_guard = logging::init(config::sunshine.min_log_level, config::sunshine.log_file);
  if (!log_deinit_guard) {
    BOOST_LOG(error) << "Logging failed to initialize"sv;
  }

  // logging can begin at this point
  // if anything is logged prior to this point, it will appear in stdout, but not in the log viewer in the UI
  // the version should be printed to the log before anything else
  BOOST_LOG(info) << "PLANK Host version: "sv << PROJECT_VERSION
                  << " commit: "sv << PROJECT_VERSION_COMMIT;

#ifdef PLANK_TRANSPORT
  if (plank_transport_abi_version() != PLANK_TRANSPORT_ABI_VERSION) {
    BOOST_LOG(fatal) << "PLANK native transport ABI mismatch"sv;
    return 9;
  }
  BOOST_LOG(info) << "PLANK native transport ABI "sv
                  << plank_transport_abi_version() << " is available"sv;
#endif

  // Log publisher metadata
  log_publisher_data();

  // Log modified_config_settings
  config::log_config_settings(config::modified_config_settings, false);
  config::modified_config_settings.clear();

#ifdef __linux__
  auto supervisor_control = plank::session::start_supervisor_control(
    [](std::uint64_t generation) {
      BOOST_LOG(info) << "Desktop attachment changed to generation "sv << generation;
      mail::man->event<std::uint64_t>(mail::desktop_reattach)->raise(generation);
    },
    [] {
      BOOST_LOG(info) << "Opening authenticated desktop; notifying the connected client"sv;
      session_stream::notify_desktop_handoff();
      std::raise(SIGTERM);
    }
  );
  if (getenv("PLANK_SESSION_CONTROL_FD") != nullptr && !supervisor_control) {
    BOOST_LOG(fatal) << "Supervisor desktop control channel was rejected"sv;
    return 8;
  }
#endif

  if (!config::sunshine.cmd.name.empty()) {
    auto fn = cmd_to_func.find(config::sunshine.cmd.name);
    if (fn == std::end(cmd_to_func)) {
      BOOST_LOG(fatal) << "Unknown command: "sv << config::sunshine.cmd.name;

      BOOST_LOG(info) << "Possible commands:"sv;
      for (auto &[key, _] : cmd_to_func) {
        BOOST_LOG(info) << '\t' << key;
      }

      return 7;
    }

    return fn->second(argv[0], config::sunshine.cmd.argc, config::sunshine.cmd.argv);
  }


  task_pool.start(1);

  // Create signal handler after logging has been initialized
  auto shutdown_event = mail::man->event<bool>(mail::shutdown);
  on_signal(SIGINT, [&force_shutdown, shutdown_event]() {
    BOOST_LOG(info) << "Interrupt handler called"sv;

    auto task = []() {
      BOOST_LOG(fatal) << "10 seconds passed, yet Sunshine's still running: Forcing shutdown"sv;
      logging::log_flush();
      lifetime::debug_trap();
    };
    force_shutdown = task_pool.pushDelayed(task, 10s).task_id;

    // Break out of the main loop
    shutdown_event->raise(true);

  });

  on_signal(SIGTERM, [&force_shutdown, shutdown_event]() {
    BOOST_LOG(info) << "Terminate handler called"sv;

    auto task = []() {
      BOOST_LOG(fatal) << "10 seconds passed, yet Sunshine's still running: Forcing shutdown"sv;
      logging::log_flush();
      lifetime::debug_trap();
    };
    force_shutdown = task_pool.pushDelayed(task, 10s).task_id;

    // Break out of the main loop
    shutdown_event->raise(true);

  });


  // If any of the following fail, log an error and continue so the failure is diagnosable.

  auto platf_deinit_guard = platf::init();
  if (!platf_deinit_guard) {
    BOOST_LOG(error) << "Platform failed to initialize"sv;
  }

  auto proc_deinit_guard = proc::init();
  if (!proc_deinit_guard) {
    BOOST_LOG(error) << "Proc failed to initialize"sv;
  }

  auto input_deinit_guard = input::init();
  if (!input_deinit_guard) {
    BOOST_LOG(fatal) << "Input backend failed to initialize"sv;
    return -1;
  }

  if (video::probe_encoders()) {
    BOOST_LOG(error) << "Video failed to find working encoder"sv;
  }

  if (http::init()) {
    BOOST_LOG(fatal) << "HTTP interface failed to initialize"sv;


    return -1;
  }

  std::unique_ptr<platf::deinit_t> mDNS;
  const bool mdns_discovery_enabled = config::sunshine.mdns_discovery;
  if (!mdns_discovery_enabled) {
    BOOST_LOG(info) << "PLANK mDNS advertisement is disabled"sv;
  }
  auto sync_mDNS = std::async(std::launch::async, [&mDNS, mdns_discovery_enabled]() {
    if (mdns_discovery_enabled) {
      mDNS = platf::publish::start();
    }
  });

  // FIXME: Temporary workaround: Simple-Web_server needs to be updated or replaced
  if (shutdown_event->peek()) {
    return lifetime::desired_exit_code;
  }

  std::jthread httpThread {nvhttp::start};
  std::jthread nativeSessionThread {session_stream::start};


  mainThreadLoop(shutdown_event);

  httpThread.join();
  nativeSessionThread.join();

  task_pool.stop();
  task_pool.join();


  return lifetime::desired_exit_code;
}
