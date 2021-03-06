/* Copyright 2015-2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <openvpn/openvpn-plugin.h>

#include "config.h"
#include "ldapauth.h"

/* static const char *OAL_NAME = "openvpn-ldap-auth"; */

/**
 * Given an environmental variable name, search
 * the envp array for its value, returning it
 * if found or NULL otherwise.
 */
static const char *
get_env (const char *name, const char *envp[]) {
  const int namelen = strlen(name);
  const char *cp;
  short int i;

  if (!envp)
    return NULL;

  for (i = 0; envp[i]; i++) {
	  if (strncmp(envp[i], name, namelen) == 0) {
      cp = envp[i] + namelen;
	    if (*cp == '=')
	      return cp + 1;
    }
	}

  return NULL;
}

OPENVPN_EXPORT int
openvpn_plugin_min_version_required_v1(void) { return 1; }

OPENVPN_EXPORT openvpn_plugin_handle_t
openvpn_plugin_open_v1 (unsigned int *type_mask,
                        const char *argv[],
                        const char *envp[])
{
  oal_config_t *config;

  if (!argv[1]) {
    fprintf(stderr, "no config provided");
    return NULL;
  }

  config = (oal_config_t *) calloc (1, sizeof (oal_config_t));

  if (parse_config(config, argv[1]) != 0) {
    fprintf(stderr, "config parser failed: %s", config->error);
    return NULL;
  }

  if (check_config(config) != 0) {
    fprintf(stderr, "config check failed: %s", config->error);
    return NULL;
  }

  *type_mask = OPENVPN_PLUGIN_MASK (OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY);
  return (openvpn_plugin_handle_t) config;
}

OPENVPN_EXPORT int
openvpn_plugin_func_v1 (openvpn_plugin_handle_t handle,
                        const int type,
                        const char *argv[],
                        const char *envp[])
{
  oal_config_t *config = (oal_config_t *) handle;
  short int rc = 0;

  /* get username/password from envp string array */
  const char *username = get_env("username", envp);
  const char *password = get_env("password", envp);

  if (type == OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY) {
    /* check entered username/password against what we require */
    rc = oal_check_cred(config, username, password);
    switch (rc) {
      case 1 : return OPENVPN_PLUGIN_FUNC_SUCCESS; break;
      case 0 : return OPENVPN_PLUGIN_FUNC_ERROR;   break;
      default :
        fprintf(stderr, "auth error: %s", config->error);
        break;
    }
  }

  return OPENVPN_PLUGIN_FUNC_ERROR; /* never reached */
}

OPENVPN_EXPORT void
openvpn_plugin_close_v1 (openvpn_plugin_handle_t handle)
{
  oal_config_t *config = (oal_config_t *) handle;
  free(config->bindurls);
  free(config->binddn);
  free(config->bindpass);
  free(config->basedn);
  free(config->userfilter);
  free(config);
}
