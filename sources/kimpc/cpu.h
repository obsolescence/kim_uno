uint8_t read6502(uint16_t address);
void write6502(uint16_t address, uint8_t value);

//a few general functions used by various other functions
void push16(uint16_t pushval);
void push8(uint8_t pushval);
uint16_t pull16(void);
uint8_t pull8(void);
void reset6502(void);
void initKIM(void);
void loadTestProgram(void);

//addressing mode functions, calculates effective addresses
void imp(void); //implied
void acc(void); //accumulator
void imm(void); //immediate
void zp(void); //zero-page
void zpx(void); //zero-page,X
void zpy(void); //zero-page,Y
void rel(void); //relative for branch ops (8-bit immediate value, sign-extended)
void abso(void); //absolute
void absx(void); //absolute,X
void absy(void); //absolute,Y
void ind(void); //indirect
void indx(void); // (indirect,X)
void indy(void); // (indirect),Y
static uint16_t getvalue(void);
static uint16_t getvalue16(void);
void putvalue(uint16_t saveval);

//instruction handler functions
void adc(void);
void op_and(void);
void asl(void);
void bcc(void);
void bcs(void);
void beq(void);
void op_bit(void);
void bmi(void);
void bne(void);
void bpl(void);
void brk(void);
void bvc(void);
void bvs(void);
void clc(void);
void cld(void);
void cli(void);
void clv(void);
void cmp(void);
void cpx(void);
void cpy(void);
void dec(void);
void dex(void);
void dey(void);
void eor(void);
void inc(void);
void inx(void);
void iny(void);
void jmp(void);
void jsr(void);
void lda(void);
void ldx(void);
void ldy(void);
void lsr(void);
void nop(void);
void ora(void);
void pha(void);
void php(void);
void pla(void);
void plp(void);
void rol(void);
void ror(void);
void rti(void);
void rts(void);
void sbc(void);
void sec(void);
void sed(void);
void sei(void);
void sta(void);
void stx(void);
void sty(void);
void tax(void);
void tay(void);
void tsx(void);
void txa(void);
void txs(void);
void tya(void);

void nmi6502(void);
void irq6502(void);

void exec6502(int32_t tickcount);

uint16_t getpc(void);
uint8_t getop(void);

