set pagination off
set logging file .task5tmp/gdb.log
set logging overwrite on
set logging enabled on
break btstack/core/tests/test_btstack_workers.c:106
commands
silent
printf "wait state=%p fixture=%p id=%d mask=%u notified=%u active=%d\n", state, &fixture, id, mask, state->notified[id], entry_active[id]
continue
end
run
