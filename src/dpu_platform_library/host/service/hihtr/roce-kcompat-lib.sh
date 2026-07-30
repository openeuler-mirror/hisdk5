#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Copyright(c) 2025 Huawei Technologies Co., Ltd

# Example of using the gen function
# Required Field []
# Optional Items ()
# gen DEFINE if [fun|enum|struct|macro|typedef|symbol] NAME (absent) in <list-of-files>
# gen DEFINE if [fun|enum|struct|macro|typedef|symbol] NAME [matches|lacks] PATTERN in <list-of-files>
# gen DEFINE if [fun] NAME [countparam] [number] in <list-of-files>
# gen DEFINE if [method] method_name of NAME [matches|lacks] PATTERN in <list-of-files>
# gen DEFINE if [method] method_name of NAME [matches|lacks] (fun_param|return_type) PATTERN in <list-of-files>
# gen DEFINE if [method] method_name of NAME (absent) in <list-of-files>
# gen DEFINE if [method] method_name of NAME [countparam] [number] in <list-of-files>

set -e

function g_log_err() {
	local log_time=$(date +"%Y-%m-%d %H:%M:%S")
	echo >&2 "$log_time $@"
	exit 10
}

WB='[ \t\n]'
NI='[^A-Za-z0-9_]'
# Return to function context, don't pass return type in $1
# looks only in files specified (files[@], struct_context)
function find_fun_context() {
	test $# -ge 1
	local fun_start fun_end
	fun_start="/$WB*([(]\*)?($NI|^)$1$WB*($|[()])/"
	fun_end='/\);?$/'
	shift
	awk "
		/^$WB*\*/ {next}
		$fun_start, $fun_end
	" "$@"
}

# Return to enum context
function find_enum_context() {
	test $# -ge 2
	local enum_start enum_end
	enum_start="/^$WB*enum$WB+$1"' \{$/'
	enum_end='/\};$/'
	awk "
		/^$WB*\*/ {next}
		$enum_start, $enum_end
	" ${files[@]}
}

# Return to struct context
function find_struct_context() {
	test $# -ge 2
	local struct_start struct_end
	struct_start="/^$WB*struct$WB+$1"' \{$/'
	struct_end='/^\};$/'
	awk "
		/^$WB*\*/ {next}
		$struct_start, $struct_end
	" ${files[@]}
}

# Return to first line of macro definition
function find_macro_context() {
	test $# -ge 2
	local macro_start macro_end
	# Match only unindented macro definitions
	macro_start="/^#define$WB+$1"'([ \t\(]|$)/'
	macro_end=1 # Extract only the first row of the matched range.
	awk "
		/^$WB*\*/ {next}
		$macro_start, $macro_end
	" ${files[@]}
}

# Return to first line of $1 typedef definition
# This only handles typedefs where the name is on first line
function find_typedef_context() {
	test $# -ge 2
	local typedef_start typedef_end
	typedef_start="/^typedef$WB.*$1"'[\(\); \t\n]/'
	typedef_end='/;$/'
	awk "
		/^$WB*\*/ {next}
		$typedef_start, $typedef_end
	" ${files[@]}
}

# Return to symbol line from Module.symvers
function find_symbol_context() {
	test $# -ge 2
	local symbol_start symbol_end
	symbol_start="/^0x[0-9a-f]+\t$1\t/"
	symbol_end=1 # only one line
	awk "
		/^$WB*\*/ {next}
		$symbol_start, $symbol_end
	" ${files[@]}
}

# Return the number of function parameters
function count_fun_params() {
	local context="$1"
	local count=0

	local params=$(echo "$context" | awk 'BEGIN{ORS=" "} {print}' | sed -n 's/.*(\(.*\)).*/\1/p' | xargs)
	if [ -n "$params" ] && [ "$params" != "void" ]; then
		IFS=',' read -ra param_array <<< "$params"
		for param in "${param_array[@]}"; do
			if [ -n "$(echo "$param" | tr -d '[:space:]')" ]; then
				((count+=1))
			fi
		done
	else
		count=0
	fi
	echo $count
}

function gen() {
	test $# -ge 6 || g_log_err "$src_line: too few arguments"
	local define="" struct_name="" member_name="" func_name=""
	local src_line="${BASH_SOURCE[0]}:${BASH_LINENO[0]}"
	files=()
	local context=""
	local pattern='.'
	local seen_in=0
	local define="$1"
	local param_count=""
	local str_found_fmt=""
	[ "$2" != "if" ] && g_log_err "$src_line: 'if' keyword expected"
	local kind="$3"
	shift 3
	if [ "$kind" = "string" ]; then
		given_value="$1"
		keyword="$2"
		expected_value="$3"
		str_found_fmt="#define %s 1\n"
		if [ "${given_value}" = "${expected_value}" ]; then
			printf -- "$str_found_fmt" "$define"
		fi
		return 0
	fi
	for arg in "$@"; do
		if [ "$arg" = "in" ]; then
			seen_in=1
			continue
		fi
		if [ $seen_in -eq 1 ] && [ -e "$arg" ] ; then
			files+=("$arg")
		fi
	done
	if [ ${#files[@]} -eq 0 ]; then
		echo >&2 "[INFO] ${BASH_LINENO[0]} gen invoked, but input file is missing."
		return 0
	fi
	case "$kind" in
		fun|enum|struct|macro|typedef|symbol)
			test $# -ge 2 || g_log_err "$src_line: too few arguments"
			name="$1"
			shift
			if [[ "$1" == "matches" || "$1" == "lacks" ]]; then
				test $# -ge 2 || g_log_err "$src_line: too few arguments"
				modelops="$1"
				pattern="$2"
			elif [[ "$1" == "absent" ]]; then
				test $# -ge 2 || g_log_err "$src_line: too few arguments"
				modelops="$1"
			elif [[ "$1" == "countparam" ]]; then
				test $# -ge 2 || g_log_err "$src_line: too few arguments"
				modelops="$1"
				param_count="$2"
			else
				modelops=matches
			fi
			context="$(find_${kind}_context "$name" "${files[@]}")"
			process_context "$context" "$define" "$pattern" "$modelops" "$param_count"
			;;
		method)
			test $# -ge 5 || g_log_err "$src_line: too few arguments"
			member_name="$1"
			pattern="$1"
			[ "$2" != of ] && g_log_err "$src_line: 'of' keyword expected"
			struct_name="$3"
			shift 3
			if [[ "$1" == "matches" || "$1" == "lacks" ]]; then
				modelops="$1"
				pattern="$2"
				if [[ "$2" == "return_type" || "$2" == "fun_param" ]]; then
					pattern="$3"
				fi
			elif [[ "$1" == "countparam" ]]; then
				modelops="$1"
				param_count="$2"
			elif [[ "$1" == "absent" ]]; then
				modelops="$1"
			else
				modelops=matches
			fi
			local struct_context=""
			struct_context="$(find_struct_context "$struct_name" "${files[@]}")"
			context="$(find_fun_context "$member_name" <<< "$struct_context")"
			process_context "$context" "$define" "$pattern" "$modelops" "$param_count"
			;;
		*)
			g_log_err "$src_line: unknown kind" ;;
	esac
}

process_context() {
	local context="$1" define="$2" pattern="$3" modelops="$4" param_count="$5"
	local found=0 not_empty=0
	if [ "$modelops" = "countparam" ]; then
		local actual_param_count=$(count_fun_params "$context")
		if [[ "$param_count" =~ ^[0-9]+$ ]] && [ "$actual_param_count" -eq "$param_count" ]; then
			found=1
		fi
	else
		# Use awk to check whether the given pattern matches in $context
		read found not_empty <<< "$(awk -v pattern="$pattern" '
		BEGIN {
			NI = "[^A-Za-z0-9_]"
			if (!match(pattern, NI "$"))
				pattern = pattern "(" NI "|$)"
			pattern = "(^|" NI ")" pattern
			found = 0
			not_empty = 0
		}
		/./ { not_empty = 1 } # mark context as non-empty
		$0 ~ pattern { found = 1 } # mark if the pattern matches
		END {
			print found, not_empty
		}
		' <<< "$context")"
	fi
	format_result "$define" "$found" "$not_empty" "$modelops" 
}

format_result() {
	local define="$1" found="$2" not_empty="$3" modelops="$4"
	local found_fmt 

	found_fmt="#define %s 1\n";

	if { [ "$modelops" = lacks ] && [ "$found" -eq 0 ] && [ "$not_empty" -eq 1 ]; } ||
	{ [ "$modelops" = matches ] && [ "$found" -eq 1 ]; } ||
	{ [ "$modelops" = absent ] && [ "$found" -eq 0 ]; } ||
	{ [ "$modelops" = countparam ] && [ "$found" -eq 1 ]; }
	then
		printf "$found_fmt" "$define"
	fi
}

find_config_net_devlink() {
	local -a config_files=(
		"$KOBJ/include/generated/autoconf.h"
		"$KOBJ/include/linux/autoconf.h"
		"$KOBJ/.config"
	)
	local file

	echo >&2 "${KSRC-}"
	if ! [ -d "${KSRC-}" ]; then
		echo >&2 "Error: KSRC not valid"
		exit 13
	fi

	for file in "${config_files[@]}"; do
		if [ -f "$file" ]; then
			echo >&2 "find config file: $file"
			config_file=$(realpath "${file-}")
			if grep -qE "^(#define )?CONFIG_NET_DEVLINK((_MODULE)? 1|=m|=y)$" "$config_file"; then
				echo >&2 "CONFIG_NET_DEVLINK Found"
				return 0
			else
				return 1
			fi
		fi
	done
	return 1
}

# return true/false instead of generating output.
function check() {
	[[ "$(gen CHECK if "$@")" = "#define CHECK 1" ]]
}