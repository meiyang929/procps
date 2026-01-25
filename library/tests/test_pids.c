/*
 * libprocps - Library to read proc filesystem
 * Tests for pids library calls
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "pids.h"
#include "tests.h"

enum pids_item items[] = { PIDS_ID_PID, PIDS_ID_PID };
enum pids_item items2[] = { PIDS_ID_PID, PIDS_VM_RSS };
enum pids_item items_tid[] = { PIDS_ID_TID };
static int cmp_int(const void *a, const void *b);

int check_pids_new_nullinfo(void *data)
{
    testname = "procps_pids_new() info=NULL returns -EINVAL";
    return (procps_pids_new(NULL, items, 0) == -EINVAL);
}

int check_pids_new_toomany(void *data)
{
    struct pids_info *info;
    testname = "procps_pids_new() too many items returns -EINVAL";
    return (procps_pids_new(&info, items, 1) == -EINVAL);
}

int check_pids_new_and_unref(void *data)
{
    struct pids_info *info = NULL;
    testname = "procps_pids new then unref";
    return ( (procps_pids_new(&info, items, 2) == 0) &&
             (procps_pids_unref(&info) == 0) &&
             info == NULL);
}

int check_fatal_proc_unmounted(void *data)
{
    struct pids_info *info = NULL;
    struct pids_stack *stack;
    testname = "check_fatal_proc_unmounted";

    return ( (procps_pids_new(&info, items2, 2) == 0) &&
	    ( (stack = fatal_proc_unmounted(info, 1)) != NULL) &&
	    ( PIDS_VAL(0, s_int, stack) > 0) &&
	    ( PIDS_VAL(1, ul_int, stack) > 0));
}

static int cmp_int(const void *a, const void *b)
{
    int lhs = *(const int *)a;
    int rhs = *(const int *)b;

    if (lhs < rhs)
        return -1;
    if (lhs > rhs)
        return 1;
    return 0;
}

int check_pids_reap_unique_tid(void *data)
{
    struct pids_info *info = NULL;
    struct pids_fetch *fetch;
    int i, count;
    int *tids;
    testname = "procps_pids_reap() unique tid entries";

    if (procps_pids_new(&info, items_tid, 1) != 0)
        return 0;
    fetch = procps_pids_reap(info, PIDS_FETCH_TASKS_ONLY);
    if (!fetch) {
        procps_pids_unref(&info);
        return 0;
    }
    for (count = 0; fetch->stacks[count]; count++) {
        if (count > INT_MAX - 1) {
            procps_pids_unref(&info);
            return 0;
        }
    }
    tids = calloc(count, sizeof(int));
    if (!tids) {
        procps_pids_unref(&info);
        return 0;
    }
    for (i = 0; i < count; i++)
        tids[i] = PIDS_VAL(0, s_int, fetch->stacks[i]);
    qsort(tids, count, sizeof(int), cmp_int);
    for (i = 1; i < count; i++) {
        if (tids[i] == tids[i - 1]) {
            free(tids);
            procps_pids_unref(&info);
            return 0;
        }
    }
    free(tids);
    procps_pids_unref(&info);
    return 1;
}

TestFunction test_funcs[] = {
    check_pids_new_nullinfo,
    // skipped, ask Jim check_pids_new_toomany,
    check_pids_new_and_unref,
    check_fatal_proc_unmounted,
    check_pids_reap_unique_tid,
    NULL };

int main(int argc, char *argv[])
{
    return run_tests(test_funcs, NULL);
}
