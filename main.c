#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <jansson.h>

#include "github.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#define RKE2_REPO_URL "https://github.com/rancher/rke2"
#define K3S_REPO_URL "https://github.com/k3s-io/k3s"

#define ORG_REPO_SIZE 24
#define MAX_RELEASE_COUNT 1024
#define RFC3339_JAN_1 "2024-01-01T00:00:00Z"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define USAGE                                                                 \
    "usage: %s [-vh]\n"                                                       \
    "  -v          version\n"                                                 \
    "  -h          help\n\n"                                                  \
    "commands:\n"                                                             \
    "  new          --bin <name> create new binary application\n"             \
    "               --lib <name> create new library\n"                        \
    "  build        builds the project with the given build constraint.\n"    \
    "  config       display the current project configuration.\n"             \
    "  deps         displays the project's dependencies.\n"                   \
    "  update       retrieves newly added dependencies.\n"                    \
    "  clean        cleans the current project based on the build parameter\n"

/**
 * str_to_time converts the given date/time string into a time_t value.
 */
time_t
str_to_time(const char *str)
{
    if (str == NULL) {
        return -1;
    }

    struct tm tm_struct = {0};

    if (strptime(str, "%Y-%m-%dT%H:%M:%SZ", &tm_struct) == NULL) {
        return 1;
    }

    return mktime(&tm_struct);
}

void
f() 
{
    data = "{\"tag_name\":\"v0.21.0\",
            \"target_commitish\":\"master\",
            \"name\":\"v.21.0\",
            \"body\":\"Description of the release\",
            \"draft\":false,\"prerelease\":false,
            \"generate_release_notes\":false}";
    res = gh_client_repo_release_create("briandowns", "devops-testing", data);
    if (res->err_msg != NULL) {
        printf("%s\n", res->err_msg);
        gh_client_response_free(res);
        return 1;
    }
    printf("%s\n", res->resp);
    gh_client_response_free(res);
}

bool
check_upstream_release(const char *tag)
{
    gh_client_response_t *res = gh_client_repo_release_by_tag("kubernetes", "kubernetes", tag);
    if (res->err_msg != NULL) {
        fprintf(stderr, "%s\n", res->err_msg);
        gh_client_response_free(res);
        return false;
    }
    if (res->resp_code != 200) {
        return false;
    }
    gh_client_response_free(res);

    return true;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        printf(USAGE, STR(bin_name));
        return 1;
    }

    char *token = getenv("GITHUB_TOKEN");
    if (token == NULL || token[0] == '\0') {
        fprintf(stderr, "github token not set in environment or invalid\n");
        return 1;
    }
    gh_client_init(token);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            printf("version: %s - git: %s\n", STR(relstat_version),
                   STR(git_sha));
            break;
        }
        if (strcmp(argv[i], "-h") == 0) {
            printf(USAGE, STR(bin_name));
            break;
        }

        if (strcmp(argv[i], "upstream") == 0) {
            if (argc < 3) {
                fprintf(stderr, "upstream requires at least 1 k8s version\n");
                return 1;
            }
            uint8_t version_count = argc - 2;
            for (uint8_t i = 0; i < version_count; i++) {
                if (check_upstream_release(argv[i+2])) {
                    printf("%-10s -  " GREEN "Released\n" RESET, argv[i+2]); 
                } else {
                    printf("%-10s -  " RED "Not Released\n" RESET, argv[i+2]);
                }
            }
            break;
        }
    }

    time_t jan_1_2024 = str_to_time(RFC3339_JAN_1);
    if (jan_1_2024 == -1) {
        fprintf(stderr, "failed to parse: " RFC3339_JAN_1 "\n");
        return 1;
    }

    gh_client_pull_req_opts_t opts = {
        .state = GH_ITEM_STATE_CLOSED,
        .per_page = 100
    };
    gh_client_response_t *res;
 
    json_t *root;
    json_t *element, *label, *labels, *label_name;
    json_error_t error;

//     do {
//         res = gh_client_repo_pull_request_list("rancher", "rke2", &opts);
//         if (res->err_msg != NULL) {
//             fprintf(stderr, "%s\n", res->err_msg);
//             gh_client_response_free(res);
//             return 1;
//         }

// #ifdef DEBUG
//         printf("%s\n", res->resp);
// #endif
//         root = json_loads(res->resp, 0, &error);
//         if (!root) {
//             fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
//             return 1;
//         }

//         char *title, *created_at, *url;

//         size_t size = json_array_size(root);

//         for (size_t i = 0; i < size; i++) {
//             element = json_array_get(root, i);

//             labels = json_object_get(element, "labels");
//             size_t label_count = json_array_size(labels);

//             if (label_count > 0) {
//                 for (size_t j = 0; j < label_count; j++) {
//                     label = json_array_get(labels, j);
//                     label_name = json_object_get(label, "name");

//                     if (strcmp(json_string_value(label_name), "release") == 0) {
//                         element = json_array_get(root, i);
                        
//                         json_unpack(element, "{s:s, s:s, s:s}", "title",
//                             &title, "created_at", &created_at, "url", &url);
//                         printf("%s\n", title);   
//                     }
//                 }
//             }
//         }
        
//         if (strlen(res->next_link) != 0) {
//             strcpy(opts.page_url, res->next_link);
//         }

//         gh_client_response_free(res);
//     } while (res->next_link[0] != '\0');

    // json_decref(labels);
    // json_decref(label_name);
    // json_decref(element);
    // json_decref(elem);
    // json_decref(root);

    // gh_client_response_t *res2 = gh_client_repo_release_by_tag("rancher", "rke2", "v1.29.15+rke2r1");
    // if (res2->err_msg != NULL) {
    //     fprintf(stderr, "ERR - %s\n", res2->err_msg);
    //     gh_client_response_free(res2);
    //     return 1;
    // }
    // printf("XXX - here\n");
    // printf("%s\n", res2->resp);
    // gh_client_response_free(res2);
    gh_client_free();

    return 0;
}
