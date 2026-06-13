#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "common.h"
#include "scenario.h"
#include "simulation.h"

static void cli_show_main_menu(void) {
    puts("\n============================================================");
    puts("              PRODUCER-CONSUMER SIMULATOR");
    puts("============================================================");
    puts("Synchronization: POSIX Semaphore + Mutex");
    puts("");
    puts("1. Run Balanced scenario");
    puts("2. Run Fast Producer scenario");
    puts("3. Run Fast Consumer scenario");
    puts("4. Help");
    puts("0. Exit");
    puts("------------------------------------------------------------");
}

static void cli_show_help(void) {
    puts("\nHELP");
    puts("------------------------------------------------------------");
    puts("Choose one of the three predefined scenarios to observe how");
    puts("producer and consumer speeds affect a bounded circular buffer.");
    puts("Threads synchronize with counting semaphores and a mutex.");
    puts("Detailed logs show buffer occupancy and blocking events.");
}

static void discard_line_remainder(void) {
    int character;
    do {
        character = getchar();
    } while (character != '\n' && character != EOF);
}

static int cli_read_integer(
    const char *prompt,
    int minimum,
    int maximum
) {
    char input[128];

    for (;;) {
        char *end;
        long value;

        fputs(prompt, stdout);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return minimum;
        }
        if (strchr(input, '\n') == NULL) {
            discard_line_remainder();
            puts("Invalid input. Enter one complete number.");
            continue;
        }

        errno = 0;
        end = NULL;
        value = strtol(input, &end, 10);
        while (end != NULL && isspace((unsigned char)*end) != 0) {
            end++;
        }
        if (errno == ERANGE || value < INT_MIN || value > INT_MAX
            || end == input || end == NULL || *end != '\0'
            || value < minimum || value > maximum) {
            printf("Invalid input. Enter a number from %d to %d.\n",
                   minimum,
                   maximum);
            continue;
        }
        return (int)value;
    }
}

static int cli_read_yes_no(const char *prompt) {
    char input[32];

    for (;;) {
        char *cursor;
        char answer;

        fputs(prompt, stdout);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }
        if (strchr(input, '\n') == NULL) {
            discard_line_remainder();
            puts("Invalid input. Enter y or n.");
            continue;
        }

        cursor = input;
        while (isspace((unsigned char)*cursor) != 0) {
            cursor++;
        }
        answer = (char)tolower((unsigned char)*cursor);
        cursor++;
        while (isspace((unsigned char)*cursor) != 0) {
            cursor++;
        }
        if (*cursor == '\0' && (answer == 'y' || answer == 'n')) {
            return answer == 'y';
        }
        puts("Invalid input. Enter y or n.");
    }
}

static void cli_wait_for_enter(void) {
    char input[8];

    fputs("\nPress Enter to return to the main menu...", stdout);
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL
        && strchr(input, '\n') == NULL) {
        discard_line_remainder();
    }
}

static void cli_run_scenario(ScenarioType type) {
    SimulationConfig config = scenario_create(type);

    scenario_print(type, &config);
    if (cli_read_yes_no("\nStart simulation? [y/n]: ") == 0) {
        puts("Simulation cancelled.");
        return;
    }
    if (simulation_run(&config) != APP_SUCCESS) {
        fputs("Simulation failed. Review the error output above.\n", stderr);
    }
    cli_wait_for_enter();
}

void cli_run(void) {
    int running = 1;

    while (running != 0) {
        int choice;

        cli_show_main_menu();
        choice = cli_read_integer("Enter your choice: ", 0, 4);
        switch (choice) {
            case 1:
            case 2:
            case 3:
                cli_run_scenario((ScenarioType)choice);
                break;
            case 4:
                cli_show_help();
                cli_wait_for_enter();
                break;
            case 0:
                running = 0;
                break;
            default:
                break;
        }
    }
    puts("Goodbye.");
}
