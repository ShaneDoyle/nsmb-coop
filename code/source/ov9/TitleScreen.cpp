// Skip MvsL and Minigames buttons
ncp_repl(0x020D317C, 9, "ADDNE R1, R1, #3")
ncp_repl(0x020D319C, 9, "SUBNE R1, R1, #3")

ncp_repl(0x020D272C, 9, "MOV R0, #6") // File select to MvsL Menu
ncp_repl(0x020CD700, 9, "MOV R0, #6") // Bowser Jr. Intro to MvsL Menu
ncp_repl(0x020D3708, 9, "MOV R1, #0") // MvsL returns to Main Menu button 0
