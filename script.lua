-- Lua test program to run in the TA
x=...

x = internal_TA_call("cubic", x);
x = internal_TA_call("cubic", x);


return(x)