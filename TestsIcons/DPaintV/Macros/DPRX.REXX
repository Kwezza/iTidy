/*  dprxtest.rexx, a simple way to feed dpaint several commands  */
/*    <Ctrl-C> exits  */

address 'DPAINT.1'
options results
options failat 50

say "DPaintV ARexx Test Tool"
say "Press <Ctrl>-C <Return> to Exit."
signal on BREAK_C

do forever
  say "DPRX>"
  pull command
  interpret command
  if RC = 18 then say "Command returned: Error 18 RC_BAD_COMMAND"
  else if RC = 17 then say "Command returned: Error 17 RC_BAD_ARGS"
  else if RC = 16 then say "Command returned: Error 16 RC_MISC_DP_ERROR"
  else if RC = 15 then say "Command returned: Error 15 RC_GUI_LOCK_ERROR"
  else if RC = 0 then say "Command returned: 0 RC_OK"
  else say "Command returned: " RC
  if result ~= "RESULT" then say result
  drop result
end

BREAK_C:
exit
