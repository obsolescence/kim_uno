MicroChess on the Arduino

Compile
-------
I compiled with http://taboo.eu.org/download/6502tass.zip .
Note: this means you have to remove the first two bytes from the output .bin file, as it generated a binary with start address in first two bytes.

Microchess How-to
-----------------

It took me quite some time to understand how to operate MicroChess. Read the introduction below, then gameplay will be totally intuitive.


Play MicroChess through the serial port:

1. Initialise
=============
Start by entering C for Clear. At any later moment, you can hit C to reset the game. The board will be reprinted.
--> the bottom line will show CC CC CC to tell you C has been hit and a reset has been performed.

2. Let the computer play against itself
=======================================
Now, enter E to make the computer take the opposite side of the board.
--> the bottom line will show EE EE EE to confirm you swapped chairs. The board remains unchanged.

Then, enter P to make the computer Play its move.
--> After hitting P, deep thought happens. Every few seconds, a dot appears to indicate one more alternative move has been thought through.
--> Typically, you'll see 30 or so dots before the computer is done. So this will take a while on an emulated 6502 on 0.5 Mhz.

When the move has been decided, MicroChess prints out the board again and the bottom line now contains the move.
The bottom-line move description is 3 hex numbers.
	Number 1 left digit tells you which side's piece is moved: 0 for Black, 1 for White
	Number 1 right digit tells you what piece this is:
		 ----------------------------------------------------------------
		| 0 - KING      | 4 - King Bishop  | B - K R Pawn | C - K B Pawn |
		| 1 - Queen     | 5 - Queen Bishop | 9 - Q R Pawn | D - Q B Pawn |
		| 2 - King Rook | 6 - King Knight  | A - K N Pawn | E - Q Pawn   |
		| 3 - Queen Rook| 7 - Queen Knight | B - Q N Pawn | F - K Pawn   |
		 ----------------------------------------------------------------

	Number 2 is the FROM square (row, column).
	Number 3 is the TO square (row, column).


OK... so far so good.
You can let the computer play both sides by hitting E again, then P, then E again, then P.

3. Entering your own move
=========================
But now, assume you want to enter a move. That is done as follows:
Instead of hitting P, you enter the 2-digit FROM square and the 2-digit TO square. Yes, that's four digits.
After every digit, you'll see the board reprinted. But focus on the bottom line: the digit you press "rolls in" to the bottom line's Numbers 2 and 3. After four digits are entered, the move is defined. MicroChess figures out what's the actual piece that this involves, so the Number 1 shows what the piece is: first digit is 0 for white, 1 for black. Second digit is as per the table above. 

Once your move is complete, hit Return to lock it in (Register your move). 
--> FF appears in Number 1: you have moved the piece, so FF shows the From square is now empty.

If you like the move you entered, great. if not, just correct the wrong move by entering four more digits in a correcting move, so that the board looks like you intended it to. 

Indeed - a fundamental point to grasp: MicroChess does not check what you're doing when you move pieces around. You can move its pieces as well as your own, all you are doing here is rearranging the board for the next round of Deep Thought about the situation you've created. If you want, you can play fair. Or you can create an interesting new situation for MicroChess to ponder about. 

Think about it: this is great. You can move any piece you want on the board to create a new situation that MicroChess should play against next. You can make a normal move, or shift stuff around on the board as you wish. It's a feature, not a limitation!

So in summary: hit the row/col of the From field, then the row/col of the To field, hit Return to make the move. And move any other pieces if you want.


--> Hit P to make MicroChess Play the next move. Deep thought marked by a line of dots will follow, then an update of the board with the move of MicroChess.


4. Special moves:
=================

Castling: just move the two pieces and return after each one to make it stick.
En passant: break the move up in two moves. The mid-point being the piece you'll strike out.
Queening pawns: yes. Well. You'll just have to remember which of your pawns has been Queened. MicroChess will not Queen on its side.. (It can be done on a Kim-I by leaving the program and manipulating its memory through the Monitor. But no such options in the Arduino version. Slight imperfection).

5. Hacking the 6502 hex code to downgrade the level of play and speed up:
=========================================================================

From the manual: There are  several  sections  of  the  program  which  can  be
bypassed  in order to reduce the computer's response time in a
given  situation.   This  will  reduce  the  quality  of  play
accordingly.     The  strategy levels  and  data  changes  are
outlined below.

        ------------------------------------------------
       | LEVEL       | LOCATION | LOCATION | AVGF TIME  |
       |             |   02F2   |   018B   | PER MOVE   |
       |------------------------------------------------|
       | SUPER BLITZ |    00    |    FF    | 3 seconds  |
       | BLITZ       |    00    |    FB    | 10 seconds |
       | NORMAL      |    08    |    FB    | 100 second |
        ------------------------------------------------
 
Technical detail for the Arduino version: the above is still true. Even though the Arduino version is located from $C000 in ROM, not from $0100 in RAM, the hex codes start counting from C000. In other words, find the 2F2th and 018Bth hex code in the array in cpu.c and modify them. But if speed is your aim, maybe this is not the hardware platform you should play with :)


6.a Further reading on MicroChess:
==================================

Original manual:
http://users.telenet.be/kim1-6502/microchess/microchess.html
 Kim-I key	Arduino Serial key
     F              Return
     E              E
     C              C
     PC             P

MicroChess for serial terminal:
do NOT use http://6502.org/source/games/uchess/uchess.htm . It's an older version with some bugs.
instead, use http://benlo.com/files/Microchess6502.txt .

Changes to this source code:
- relocate code to $C000
- hack 6551 UART out, replace by $CFF1 - output character, $CFF3 - 1=key pressed, $CFF4 - ASCII code of key pressed.
- the original used the PHY instruction which is only present in newfangled 65C02s. Removed it by wasting 1 byte for temp storage.

6.b. Just to make sure
======================
All I did was to recompile MicroChess with marginal UART changes, and paste it into the above Arduino source code as an array of hex codes to replace 
http://forum.arduino.cc/index.php?topic=193216.0
