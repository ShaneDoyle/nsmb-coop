namespace CoopFixes::TileGod {

// Tile God updates during stage freeze
ncp_repl(0x0216D3E4, 54, ".int _ZN10StageActor9preUpdateEv");

} // namespace CoopFixes::TileGod
