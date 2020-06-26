-- Lua test program to run in the TA
x=...

x = internal_TA_call("cubic", x);
x = 2*x

return(x)