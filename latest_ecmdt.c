#include <jansson.h>
#include <stdio.h>

#include "github.h"

int
main(void)
{
    char *token = getenv("GITHUB_TOKEN");
    if (token == NULL || token[0] == '\0') {
        fprintf(stderr, "github token not set in environment or invalid\n");
        return 1;
    }
    gh_client_init(token);

    gh_client_response_t *res = gh_client_repo_releases_latest("rancher", "ecm-distro-tools");
    if (res->err_msg != NULL) {
        fprintf(stderr, "%s\n", res->err_msg);
        gh_client_response_free(res);
        return false;
    }
    if (res->resp_code != 200) {
        return false;
    }
    //printf("%s\n", res->resp);
    

    json_error_t error;
    json_t *root = json_loads(res->resp, 0, &error);

    const char *ecm_version;
    json_unpack(root, "{s:s}", "tag_name", &ecm_version);

    printf("%s\n", ecm_version);
    json_decref(root);

    gh_client_response_free(res);
    gh_client_free();

    return 0;
}
