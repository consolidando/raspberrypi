#include <stdio.h>
#include <stdlib.h>
#include <jwt.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

// gets iat and exp claims
// expiration time: 24 h
static void getIatExp(char *iat, char *exp, int time_size)
{
    time_t now_seconds = time(NULL);
    snprintf(iat, time_size, "%lu", now_seconds);
    snprintf(exp, time_size, "%lu", now_seconds + 24 * 60 * 60);
}

// creates an authentication token - jwt
char* jwtCreate(char *opt_key_name, char* aud, char* deviceId)
{
    size_t key_len = 0;
    FILE *fp_priv_key;
    unsigned char key[10240];    
    jwt_t *jwt = NULL;    
    char iat_time[sizeof(time_t) * 3 + 2];
    char exp_time[sizeof(time_t) * 3 + 2];
    char* out = NULL;

    fp_priv_key = fopen(opt_key_name, "r");
    key_len = fread(key, 1, sizeof(key), fp_priv_key);
    fclose(fp_priv_key);
    key[key_len] = '\0';    

    jwt_new(&jwt);
    getIatExp(iat_time, exp_time, sizeof(iat_time));
    jwt_add_grant(jwt, "iat", iat_time);
    jwt_add_grant(jwt, "exp", exp_time);
    jwt_add_grant(jwt, "aud", aud);
    jwt_add_grant(jwt, "deviceId", deviceId);
    jwt_set_alg(jwt, JWT_ALG_RS256, key, key_len);
    out = jwt_encode_str(jwt);

    jwt_free(jwt);

    return out;
}