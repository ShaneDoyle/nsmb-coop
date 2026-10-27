namespace CoopFixes::PipeCannon {

// Fix pipe cannon desync.
ncp_repl(0x020F8230, 10, "B 0x020F823C")

} // namespace CoopFixes::PipeCannon
