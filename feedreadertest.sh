#!/bin/bash

# Script for testing feedreader project
# Author: Vojtěch Dvořák (xdvora3o)

# Tests cases are located in folder described by TEST_DIR path

# Each test case has own folder with multiple files:
# TEST_FILE_NAME = file with arguments for specific test case 
#                  (or with description of test case on the first line and 
#                   arguments on the second line)
# OUPUT_FILE_NAME = file with expected output (stdout) of program
# RET_CODE_FILE_NAME = file with expected return code of program
#
# During run time is RESULT_FILE_NAME in test case folder generated 
# for real output to stdout and it is compared with expected result by diff
#
# If OUPUT_FILE_NAME or RET_CODE_FILE_NAME is missing, there is no comparison
# of expected return code or output (depends on missing file)
#
# For preserving output files (*.tmp) use -v option, otherwise thy are deleted
# For running tests with valgrind use -m option (in this case is dependent on 
# valgrind)

PROGRAM_PATH="./feedreader"

TEST_DIR="tests/*" # Path to directories with test files

TEST_FILE_NAME="test"
OUTPUT_FILE_NAME="out"
RET_CODE_FILE_NAME="ret"

RESULT_FILE_NAME="out.tmp" # File with STDOUT that was produced by the program
ERROR_FILE_NAME="err.tmp" # File with STDERR that was produced by the program
DIFF_FILE_NAME="diff.tmp" # Differences between expected and real STDOUT
VALGRIND_LOG_FILE_NAME="valgrind.tmp"

VERBOSE=0 # Default value of verbose
MEMCHECK=0 # Default value of memcheck
ALL_TESTS=0

PASSED_MSG="[ \033[0;32mPASSED\033[0m ]"
FAILED_MSG="[ \033[0;31mFAILED\033[0m ]"

TEST_TO_BE_EXEC=""

VALGRIND_CMD="valgrind"

# Initialization of global values for storing the tests summary
PASSED_NUM=0
TOTAL_NUM=0


# Parse options
function parse_options() {
    while getopts "vmhat:p:" OPT
    do
        if [ "$OPT" = "v" ]
        then
            VERBOSE=1
        elif [ "$OPT" = "m" ]
        then
            MEMCHECK=1
        elif [ "$OPT" = "h" ]
        then
            echo "Test script of feedreader program."
            echo
            echo "USAGE: ./feedreadertest.sh [OPTIONS]..."
            echo
            echo "Options:"
            echo -e "-m\tActivates memory checks (valgrind is needed)"
            echo -e "-v\tPreserves temporary files in test folders (for debugging)"
            echo -e "-h\tPrints help and ends program"
            echo -e "-t test1\tRuns only test in 'test1' folder"
            echo -e "-p path\tSpecifies the path to the program to be tested"
            echo -e "-a\tRun all the tests, do not skip the hidden test cases (suffix '_')"
            echo
            echo "Return codes:"
            echo "0 - All tests passed"
            echo "1 - There are failed tests"
            echo "2 - Errors ocurred inside test script"
            exit 0
        elif [ "$OPT" = "p" ]
        then
            PROGRAM_PATH=$OPTARG
        elif [ "$OPT" = "t" ]
        then
            TEST_TO_BE_EXEC=$OPTARG
        elif [ "$OPT" = "a" ]
        then
            ALL_TESTS=1
        fi
    done
}


function check_binaries() {
    if [ ! -x "$PROGRAM_PATH" ]
    then
        echo "Unable to execute file '${PROGRAM_PATH}'! (check the path, please)"
        exit 2
    fi


    if [ $MEMCHECK == 1 ]
    then
        if ! command -V "$VALGRIND_CMD" >/dev/null 2>&1
        then
            echo "Used option '-m', but unable to execute '${VALGRIND_CMD}' for memcheck!"
            exit 2
        fi
    fi
}


function test_exec() {
    echo "$1:"

    VALID_FOLDER=0
    for TEST in $1
    do
        if [ -d "$TEST" ]
        then
            if [ -f "$TEST/$TEST_FILE_NAME" ]
            then
                VALID_FOLDER=1

                if [[ "$TEST_TO_BE_EXEC" != "" &&  "$TEST_TO_BE_EXEC" != "${TEST#$1}" ]] 
                then
                    continue
                fi
                
                if [[ "${TEST: -1: 1}" == "_" && $ALL_TESTS == 0 ]] # Skip "hidden" tests
                then
                    continue
                fi

                echo -e -n "${TEST#$1}:\t"

                HEAD_LINES=`head -2 "$TEST/$TEST_FILE_NAME"`

                INIT_LINE=`echo "$HEAD_LINES" | head -1`
                ARGS=$INIT_LINE # Arguments for program in this test case

                if [[ "${INIT_LINE:0:1}" == "#" ]]
                then
                    DESCRIPTION=`echo "${INIT_LINE:1:60}" | xargs` # Remove # from start and trim string
                    ARGS=`echo "$HEAD_LINES" | tail -1` # Line with test command is second line of test file
                else
                    DESCRIPTION=""
                fi

                ERROR_FILE=$ERROR_FILE_NAME
                RESULT_FILE=$RESULT_FILE_NAME
                PROGRAM_REALPATH=$(realpath ${PROGRAM_PATH})
                cd $TEST # Go to Directory with current test

                if [ $MEMCHECK == 1 ] # Testing
                then
                    eval "${VALGRIND_CMD} --leak-check=full --log-file=\"${VALGRIND_LOG_FILE_NAME}\" ${PROGRAM_REALPATH} >${RESULT_FILE} 2>${ERROR_FILE} ${ARGS}"
                else
                    eval "${PROGRAM_REALPATH} >${RESULT_FILE} 2>${ERROR_FILE} ${ARGS}"
                fi
                RETURN_CODE=$?
                
                REASON=""
                RESULT=$PASSED_MSG
                if [ -f "$RET_CODE_FILE_NAME" ]
                then
                    read EXPECTED_RETURN_CODE < "$RET_CODE_FILE_NAME"
                    if [ "$EXPECTED_RETURN_CODE" != "$RETURN_CODE" ]
                    then
                        REASON="${REASON}Return code mismatch, Expected: $EXPECTED_RETURN_CODE Got: $RETURN_CODE\n"
                        RESULT=$FAILED_MSG
                    fi
                fi

                if [ -f "$OUTPUT_FILE_NAME" ]
                then
                    diff $RESULT_FILE $OUTPUT_FILE_NAME > $DIFF_FILE_NAME
                    if [ $? != 0 ]
                    then
                        REASON="${REASON}Different outputs! (use -v to preserve $RESULT_FILE_NAME file)\n"
                        RESULT=$FAILED_MSG
                    fi
                fi

                if [ $MEMCHECK == 1 ]
                then
                    VALGRIND_LOG_TAIL=`cat ${VALGRIND_LOG_FILE_NAME} | tail -1`
                    VALGRIND_ERROR_SUM=`echo -n ${VALGRIND_LOG_TAIL} | grep "ERROR SUMMARY: [1-9][0-9]*" -`
                    if [[ $VALGRIND_ERROR_SUM != "" ]]
                    then
                        REASON="${REASON}${VALGRIND_LOG_TAIL}\n"
                        RESULT=$FAILED_MSG
                    fi
                fi

                if [ $VERBOSE != 1 ] # Remove temporary files
                then
                    rm -f $ERROR_FILE $RESULT_FILE $DIFF_FILE_NAME $VALGRIND_LOG_FILE_NAME
                fi

                echo -e "$RESULT\t$DESCRIPTION"
                if [[ "$REASON" != "" ]]
                then
                    echo -e -n "Why: \t$REASON"
                fi

                if [[ "$RESULT" != "$FAILED_MSG" ]]
                then
                    PASSED_NUM=$(expr $PASSED_NUM + 1)
                fi

                TOTAL_NUM=$(expr $TOTAL_NUM + 1)

                cd $RETURN_PATH # Return to default directory
            fi
        fi
    done 

    for TEST in $1
    do
        if [ -d "$TEST" ]
        then
            if [ ! -f "$TEST/$TEST_FILE_NAME" ]
            then
                VALID_FOLDER=0

                FOLDER_CONTENT="$TEST/*"
                for SUB_FOLDER in $FOLDER_CONTENT
                do
                    if [ -d "${SUB_FOLDER}" ]
                    then
                        VALID_FOLDER=1
                        break
                    fi
                done

                if [ $VALID_FOLDER == 0 ]
                then
                    if [[ "$TEST_TO_BE_EXEC" != "" &&  "$TEST_TO_BE_EXEC" != "${TEST#$1}" ]] 
                    then
                        continue
                    fi

                    echo -e "\033[0;33mMandatory '$TEST_FILE_NAME' file nor subdirectory was not found in '$TEST'!\033[0m"
                else
                    test_exec "$TEST/*"
                fi
            fi
        fi
    done
} # End of function test_exec


# Main body of the test script

parse_options $*

check_binaries

date
echo "Running tests of feedreader:"

RETURN_PATH=$(realpath .)
test_exec "$TEST_DIR"

# Printing summary
echo -n "Summary: ${PASSED_NUM}/${TOTAL_NUM} tests passed "

if [ $PASSED_NUM == $TOTAL_NUM ]
then
    echo -e "\t\033[1;32mWell done! All tests passed!\033[0m"
    exit 0
else
    echo ""
    exit 1
fi

# End of the main body