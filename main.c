#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <jansson.h>

#include "github.h"

#define RKE2_REPO_URL "https://github.com/rancher/rke2"
#define K3S_REPO_URL "https://github.com/k3s-io/k3s"

#define ORG_REPO_SIZE 24
#define MAX_RELEASE_COUNT 1024
#define RFC3339_JAN_1 "2024-01-01T00:00:00Z"

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

int
main(int argv, char **argc)
{
    char *token = getenv("GITHUB_TOKEN");
    if (token == NULL || token[0] == '\0') {
        fprintf(stderr, "github token not set in environment or invalid\n");
        return 1;
    }
    gh_client_init(token);

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
    json_t *element, *elem, *label, *labels, *label_name;
    json_error_t error;

    do {
        res = gh_client_repo_pull_request_list("rancher", "rke2", &opts);
        if (res->err_msg != NULL) {
            fprintf(stderr, "%s\n", res->err_msg);
            gh_client_response_free(res);
            return 1;
        }
        printf("XXX - next link: %s\n", res->next_link);
#ifdef DEBUG
        printf("%s\n", res->resp);
#endif
        root = json_loads(res->resp, 0, &error);
        if (!root) {
            fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
            return 1;
        }

        char *title, *created_at, *url;

        size_t size = json_array_size(root);

        for (int i = 0; i < size; i++) {
            element = json_array_get(root, i);

            labels = json_object_get(element, "labels");
            size_t label_count = json_array_size(labels);

            if (label_count > 0) {
                for (int j = 0; j < label_count; j++) {
                    label = json_array_get(labels, j);
                    label_name = json_object_get(label, "name");

                    if (strcmp(json_string_value(label_name), "release") == 0) {
                        element = json_array_get(root, i);
                        
                        json_unpack(element, "{s:s, s:s, s:s}", "title",
                            &title, "created_at", &created_at, "url", &url);
                        printf("%s\n", title);   
                    }
                }
            }
        }
        
        if (strlen(res->next_link) != 0) {
            opts.page_url = res->next_link;
        }

        printf("%s\n", res->next_link);
    } while (res->next_link[0] != '\0');

    // json_decref(labels);
    // json_decref(label_name);
    // json_decref(element);
    // json_decref(elem);
    json_decref(root);

    return 0;
}
