## Infrastructure (do these first, everything else leans on them)

- [ ] **Register-pair (RP) field decoder** — bits 5-4 of opcode, `00=BC 01=DE 10=HL 11=SP` (or `PSW` for PUSH/POP). Mirrors what `get_reg`/`set_reg` did for the 3-bit field — write `get_rp`/`set_rp` (16-bit) once, reuse across LXI/DAD/INX/DCX/PUSH/POP.
- [ ] **Condition-code (CCC) field decoder** — bits 5-3 of opcode on conditional jump/call/return, `000=NZ 001=Z 010=NC 011=C 100=PO 101=PE 110=P 111=M`. One `check_condition(uint8_t cc)` function, reused by `Jccc`/`Cccc`/`Rccc` blocks.
- [ ] **16-bit stack push/pop helpers** — `push16(value)` / `pop16()` against `SP`, decrementing/incrementing correctly. Needed by CALL/RET/RST/PUSH/POP.
- [ ] **PSW packing/unpacking** — flags byte layout is `S Z 0 AC 0 P 1 C` (bit 1 always set, bits 5 and 3 always clear). Needed for `PUSH PSW`/`POP PSW`.

## Bitmask-decodable blocks (same trick as MOV/ALU)

- [ ] **INR/DCR** — `00DDD100`/`00DDD101`. Read-modify-write via `get_reg`/`set_reg`. Sets Z/S/P/AC but **not** Carry (8080 quirk — don't touch Carry here).
- [ ] **MVI** — `00DDD110`, second byte is the immediate. `set_reg(d, op1)`.
- [ ] **LXI** — `00RP0001`, 16-bit immediate into a register pair via `set_rp`.
- [ ] **DAD** — `00RP1001`, `HL += rp`, sets Carry only (no other flags).
- [ ] **INX/DCX** — `00RP0011`/`00RP1011`, increment/decrement a register pair, no flags affected at all.
- [ ] **PUSH/POP** — `11RP0101`/`11RP0001`, where RP's `11` slot means PSW instead of SP here (different table than LXI/DAD/INX/DCX — don't reuse the same RP mapping blindly).
- [ ] **Conditional jumps** — `11CCC010`.
- [ ] **Conditional calls** — `11CCC100`.
- [ ] **Conditional returns** — `11CCC000`.
- [ ] **RST** — `11NNN111`, NNN selects vector `N*8`.

## Individual opcodes (no shared field pattern, one-off handlers)

- [ ] `JMP`, `CALL`, `RET` (unconditional)
- [ ] `STA`/`LDA` (direct addressing, A ↔ memory)
- [ ] `SHLD`/`LHLD` (direct addressing, HL ↔ memory)
- [ ] `XCHG` (swap DE/HL)
- [ ] `XTHL` (swap HL with top of stack)
- [ ] `SPHL` (SP = HL)
- [ ] `PCHL` (PC = HL)
- [ ] `RLC`/`RRC`/`RAL`/`RAR` (accumulator rotates, watch Carry vs old-bit-into-Carry semantics — RLC/RRC vs RAL/RAR differ in whether Carry participates in the rotate or just captures the bit shifted out)
- [ ] `CMA` (complement A, no flags affected)
- [ ] `STC`/`CMC` (set/complement Carry, no other flags)
- [ ] `DAA` (decimal adjust — the gnarliest one, get the algorithm from a reference table rather than deriving it)
- [ ] `EI`/`DI` (interrupt enable flag — only matters once you implement interrupts)
- [ ] `IN`/`OUT` (port I/O — needs an I/O abstraction decision first: stub it, or wire to something real)

## Validation milestones

- [ ] Run `8080PRE.COM` to completion (preliminary self-test, exercises a useful but not exhaustive opcode subset) and confirm it reports success rather than looping/jumping somewhere unexpected.
- [ ] Run `TEST.COM` / `CPUTEST.COM` (more exhaustive 8080 diagnostics) once you've got CALL/RET/conditional jumps working — these are the standard "did I actually implement this CPU correctly" ROMs people use.
- [ ] Add an instruction-count or cycle-count tripwire so an infinite loop from a bad jump doesn't run forever silently.