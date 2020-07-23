/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

//#include "os/mynewt.h"
//#include "streamer/streamer.h"

struct os_eventq;
struct streamer;
struct shell_cmd;

/** Command IDs in the "shell" newtmgr group. */
#define SHELL_SMP_OP_EXEC      0

/** @brief Callback called when command is entered.
 *
 *  @param argc Number of parameters passed.
 *  @param argv Array of option strings. First option is always command name.
 *
 * @return 0 in case of success or negative value in case of error.
 */
typedef int (*shell_cmd_func_t)(int argc, char *argv[]);

/**
 * @brief Callback for "extended" shell commands.
 *
 * @param cmd                   The shell command being executed.
 * @param argc                  Number of arguments passed.
 * @param argv                  Array of option strings. First option is always
 *                                  command name.
 * @param streamer              The streamer to write shell output to.
 *
 * @return                      0 on success; SYS_E[...] on failure.
 */
typedef int (*shell_cmd_ext_func_t)(const struct shell_cmd *cmd,
                                    int argc, char *argv[],
                                    struct streamer *streamer);

struct shell_param {
    const char *param_name;
    const char *help;
};

struct shell_cmd_help {
    const char *summary;
    const char *usage;
    const struct shell_param *params;
};

struct shell_cmd {
    uint8_t sc_ext : 1; /* 1 if this is an extended shell comand. */
    union {
        shell_cmd_func_t sc_cmd_func;
        shell_cmd_ext_func_t sc_cmd_ext_func;
    };

    const char *sc_cmd;
    const struct shell_cmd_help *help;
};

struct shell_module {
    const char *name;
    const struct shell_cmd *commands;
};

#if MYNEWT_VAL(SHELL_CMD_HELP)
#define SHELL_HELP_(help_) (help_)
#else
#define SHELL_HELP_(help_)
#endif

/**
 * @brief constructs a legacy shell command.
 */
#define SHELL_CMD(cmd_, func_, help_) {         \
    .sc_ext = 0,                                \
    .sc_cmd_func = func_,                       \
    .sc_cmd = cmd_,                             \
    .help = SHELL_HELP_(help_),                 \
}

/**
 * @brief constructs an extended shell command.
 */
#define SHELL_CMD_EXT(cmd_, func_, help_) {     \
    .sc_ext = 1,                                \
    .sc_cmd_ext_func = func_,                   \
    .sc_cmd = cmd_,                             \
    .help = SHELL_HELP_(help_),                 \
}

/** @brief Register a shell_module object
 *
 *  @param shell_name Module name to be entered in shell console.
 *
 *  @param shell_commands Array of commands to register.
 *  The array should be terminated with an empty element.
 */
int shell_register(const char *shell_name,
                   const struct shell_cmd *shell_commands);

/** @brief Optionally register an app default cmd handler.
 *
 *  @param handler To be called if no cmd found in cmds registered with
 *  shell_init.
 */
void shell_register_app_cmd_handler(shell_cmd_func_t handler);

/** @brief Callback to get the current prompt.
 *
 *  @returns Current prompt string.
 */
typedef const char *(*shell_prompt_function_t)(void);

/** @brief Optionally register a custom prompt callback.
 *
 *  @param handler To be called to get the current prompt.
 */
void shell_register_prompt_handler(shell_prompt_function_t handler);

/** @brief Optionally register a default module, to avoid typing it in
 *  shell console.
 *
 *  @param name Module name.
 */
void shell_register_default_module(const char *name);

/** @brief Optionally set event queue to process shell command events
 *
 *  @param evq Event queue to be used in shell
 */
void shell_evq_set(struct dpl_eventq *evq);

/**
 * @brief Processes a set of arguments and executes their corresponding shell
 * command.
 *
 * @param argc                  The argument count (including command name).
 * @param argv                  The argument list ([0] is command name).
 * @param streamer              The streamer to send output to.
 *
 * @return                      0 on success; SYS_E[...] on failure.
 */
int shell_exec(int argc, char **argv, struct streamer *streamer);

#if MYNEWT_VAL(SHELL_SMP)
struct dpl_mbuf;
typedef int (*shell_nlip_input_func_t)(struct dpl_mbuf *, void *arg);
int shell_nlip_input_register(shell_nlip_input_func_t nf, void *arg);
int shell_nlip_output(struct dpl_mbuf *m);
#endif

#if MYNEWT_VAL(SHELL_COMPAT)
int shell_cmd_register(const struct shell_cmd *sc);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_H__ */
