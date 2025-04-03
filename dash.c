#include <ncurses.h>
#include <stdint.h>
#include <string.h>

#define STR1(x) #x
#define STR(x) STR1(x)

char *versions[] = {
    "v1.29.16+rke2r1",
    "v1.30.12+rke2r1",
    "v1.31.9+rke2r1",
    "v1.32.3+rke2r1"
};

int
main(void)
{
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // WINDOW *subwin(WINDOW *orig, int nlines, int ncols, int begin_y, int begin_x);

    WINDOW *main_win = newwin(0, 0, 0, 0);
    box(main_win, 0, 0);
    refresh();
    
    WINDOW *version_menu_win = subwin(main_win, 8, 21, 1, 1);
    box(version_menu_win, 0, 0);
    mvwprintw(version_menu_win, 0, 4, "%s Versions", "RKE2");
    wrefresh(version_menu_win);

    WINDOW *relstat_version_win = subwin(main_win, 3, max_x-2, max_y-4, 1);
    box(relstat_version_win, 0, 0);
    mvwprintw(relstat_version_win, 1, 1, "ECM Distro Tools: Dashboard - version: %s - git: %s", STR(relstat_version), STR(git_sha));
    wrefresh(relstat_version_win);

    WINDOW *release_display_win = subwin(main_win, max_y-5, 214, 1, 23);
    box(release_display_win, 0, 0);  

    wrefresh(main_win);

    int highlight = 0;
    int choice = 0;

    while (1) {
        for (int i = 0; i < 4; i++) {
            if (i == highlight) {
                wattron(version_menu_win, A_REVERSE);
            }

            mvwprintw(version_menu_win, i + 2, 2, "%s", versions[i]);
            if (i == highlight) {
                wattroff(version_menu_win, A_REVERSE);
            }
        }
        wrefresh(version_menu_win);

        switch (getch()) {
            case KEY_DOWN:
                highlight = (highlight + 1) % 4;
                break;
            case KEY_UP:
                highlight = (highlight + 4 - 1) % 4;
                break;
            case 10:
                if (strcmp(versions[highlight], "Exit") == 0) {
                    choice = -1; // Signal exit
                    goto exit;
                }

                mvwprintw(stdscr, 20, 0, "You selected: %s", versions[highlight]);
                mvwprintw(release_display_win, 1, 1, "%s Release Information", versions[highlight]);
                wrefresh(release_display_win);
                wrefresh(stdscr);
                getch(); // Wait for a key press
                break;
            default:
                wrefresh(version_menu_win);
                break;
        }
    }

    // uint8_t version_start_y = 2;
    // for (uint8_t i = 0; i < 4; i++, version_start_y++) {
    //     mvwprintw(version_menu_win, version_start_y, 3, "%s", versions[i]);
    // }

    

    getch();

exit:
    delwin(main_win);
    endwin();

    return 0;
}


// get latest version of ECM distro tools
