#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <jansson.h>

#include "github.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#define RKE2_REPO_URL    "https://github.com/rancher/rke2"
#define RANCHER_REPO_URL "https://github.com/rancher/rancher"
#define CLI_REPO_URL     "https://github.com/rancher/cli"
#define K3S_REPO_URL     "https://github.com/k3s-io/k3s"

#define RELEASE_BRANCH_LEN 14
#define ORG_REPO_SIZE      24
#define MAX_RELEASE_COUNT  1024
#define RFC3339_JAN_1      "2024-01-01T00:00:00Z"

#define K3S     "k3s"
#define RKE2    "rke2"
#define RANCHER "rancher"

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
    "  build        builds the project with the given build constraint.\n"

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

static inline const char*
org_from_repo(const char *repo)
{
    if (strcmp(repo, RKE2) == 0 || strcmp(repo, RANCHER) == 0) {
        return RANCHER;
    }
    if (strcmp(repo, K3S)) {
        return "k3s-io";
    }

    return "";
}

#define NEW_RELEASE_TMP "{\"tag_name\":\"%s\"," \
    "\"target_commitish\":\"master\"," \
    "\"name\":\"%1$s\"," \
    "\"body\":\"Description of the release\"," \
    "\"draft\":false,\"prerelease\":%s," \
    "\"generate_release_notes\":%s}"

static inline bool
valid_tag(const char *tag)
{
    if (tag[0] == '\0' || tag[0] != 'v') {
        return false;
    }

    uint count = 0;
    for (size_t i = 0; i < strlen(tag); i++) {
        if (tag[i] == '.') {
            count++;
        }
    }
    
    if (count != 2) {
        return false;
    }

    return true;
}

static inline int
tag_to_release_branch(char *tag, char *release)
{
    const char *delim = ".";
    uint8_t count = 0;
    char mv[3] = {0};
    char tag_copy[24] = {0};
    strncpy(tag_copy, tag, 24);

    char *token = strtok(tag, delim);
    while (token != NULL) {
        count++;

        token = strtok(NULL, delim);
        if (count == 1) {
            strncpy(mv, token, 3);
        }
    }

    uint8_t minor_version = atoi(mv);

    switch(minor_version) {
    case 29:
        strncpy(release, "release-1.29", RELEASE_BRANCH_LEN); // LTS
        break;
    case 30:
        strncpy(release, "release-1.30", RELEASE_BRANCH_LEN);
        break;
    case 31:
        strncpy(release, "release-1.31", RELEASE_BRANCH_LEN);
        break;
    case 32:
        strncpy(release, "release-1.32", RELEASE_BRANCH_LEN);
        break;
    case 33:
        strncpy(release, "release-1.33", RELEASE_BRANCH_LEN);
        break;
    default:
        printf("error: unrecognized version: %s\n", tag_copy);
        return 1;
    }

    return 0;
}

// void
// f() 
// {
//     char data[4096] = {0};
//     sprintf(data, NEW_RELEASE_TMP, "tag");
//     gh_client_response_t *res = gh_client_repo_release_create("briandowns", "devops-testing", data);
//     if (res->err_msg != NULL) {
//         printf("%s\n", res->err_msg);
//         gh_client_response_free(res);
//         return 1;
//     }
//     printf("%s\n", res->resp);
//     gh_client_response_free(res);
// }

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

        if (strcmp(argv[i], "tag") == 0) {
            if (argc < 4) {
                fprintf(stderr, "tag requires repo and version\n");
                return 1;
            }

            char url[GH_MAX_URL_LEN] = {0};
            if (strcmp(argv[2], "rke2") == 0) {
                strncpy(url, RKE2_REPO_URL, GH_MAX_URL_LEN);
                char release_branch[RELEASE_BRANCH_LEN] = {0};
                if (tag_to_release_branch(argv[3], release_branch) != 0) {
                    return 1;
                }
                printf("%s\n", release_branch);
            }
            if (strcmp(argv[2], "rancher") == 0) {
                strncpy(url, RANCHER_REPO_URL, GH_MAX_URL_LEN);
            }
            if (strcmp(argv[2], "cli") == 0) {
                strncpy(url, CLI_REPO_URL, GH_MAX_URL_LEN); 
            }
            if (strcmp(argv[2], "k3s") == 0) {
                strncpy(url, K3S_REPO_URL, GH_MAX_URL_LEN);
                char release_branch[RELEASE_BRANCH_LEN] = {0};
                if (tag_to_release_branch(argv[3], release_branch) != 0) {
                    return 1;
                }
                printf("%s\n", release_branch);
            }
            
        }
    }

    // time_t jan_1_2024 = str_to_time(RFC3339_JAN_1);
    // if (jan_1_2024 == -1) {
    //     fprintf(stderr, "failed to parse: " RFC3339_JAN_1 "\n");
    //     return 1;
    // }

    // gh_client_pull_req_opts_t opts = {
    //     .state = GH_ITEM_STATE_CLOSED,
    //     .per_page = 100
    // };
    // gh_client_response_t *res;
 
    // json_t *root;
    // json_t *element, *label, *labels, *label_name;
    // json_error_t error;

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
