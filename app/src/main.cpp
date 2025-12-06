/*
 * Backup utility.
 */

#include "ShieldClient.h"
#include "thirdparty/ctrl-c.h"

#include <iostream>
#include <algorithm>

#include <thread>
#include <chrono>

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#else
#include <locale>
#include <clocale>
#endif

#include "plog/Log.h"
#include "plog/Init.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/ConsoleAppender.h"

using namespace std;
using namespace std::literals::chrono_literals;

int
main(int argc, const char** argv)
{
    //
    // Initialize logger.
    //
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::info, &consoleAppender);

#ifdef _MSC_VER
    _setmode(_fileno(stderr), _O_WTEXT);
#else
    std::setlocale(LC_ALL, "");
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::wcerr.imbue(std::locale());
#endif

    //
    // Configuration file.
    //
    auto dir = weakly_canonical(fs::path(argv[0])).parent_path();
    auto configfile = dir / "config.protected.json";
    if (fs::exists(configfile)) {
        LOGI << "No config file found in in " << dir.string() << endl;
    }

    //
    // Create the client. It takes the name of minifilter's port.
    //
    auto client = ShieldClient(L"\\ShieldPort", configfile);

    //
    // Connect with BackupOnDelete Shield.
    //
    constexpr unsigned short MAX_NUM_TRIES = 1024;

    int sleepFor = 2;
    for (size_t i = 0; i < MAX_NUM_TRIES; i++) {
        client.connect();
        if (client.is_connected())
            break;

        //
        // Progressively increase the wait time or 32 seconds, whichever is
        // minimum.
        //
        sleepFor = min((int)32, (int)(sleepFor * 2));

        PLOGW << "Failed to connect. Will try to connect after " << sleepFor
              << "s. Trial num=" << i << " (maximum allowed " << MAX_NUM_TRIES
              << ")." << endl;

        std::this_thread::sleep_for(std::chrono::seconds(sleepFor));
    }

    //
    // If the client is still not connected after MAX_NUM_TRIES, then give up.
    // But raise an alarm first.
    //
    if (!client.is_connected()) {
        //
        // TODO: Should raise an alert.
        //
    }
    client.run();
    return 0;
}
