<div align="center">
  <a href=".">
    <img src="./imgs/logo.png" height="200"/>
  </a>
</div>

# License: <a href="https://github.com/rafid-dev/rice-2.0/blob/main/LICENSE">MIT License</a>

# Overview

Rice is a free and open source UCI chess engine written in C.

Rice is not a complete chess program and requires a <a href="https://www.chessprogramming.org/UCI#GUIs">UCI-compatible graphical user interface</a> in order to be used comfortably.

### Features
# Board Representation
* Bitboards
* Fancy magics for movegen

# Search

* Negamax
* Alpha beta pruning
* Quiescence search
* Null move pruning (NMP)
* Static null move pruning aka Reverse Futility Pruning
* PVS (Principal Variation Search)
* ZWS (Zero Window Search)
* LMR (Late Move Reduction)
* SEE Pruning (Static Exchange Evaluation Pruning)
* Movecount Pruning/LMP (Late Move Pruning)
* Transposition Table cutoffs and move ordering
* History, killers and MVVLVA Move ordering

# Evaluation
* NNUE (Efficiently updateable neural network)

# Acknowledgements

I would specially like to thank <a href="https://github.com/Disservin">Disservin</a>, author of <a href="https://github.com/Disservin/Smallbrain">Smallbrain</a> for his <a href="https://github.com/Disservin/chess-library">chess library in c++</a> which is used in Rice.

I would also like to specially thank <a href="https://github.com/TheBlackPlague">Shaheryar Sohail</a>, author of <a href="https://github.com/TheBlackPlague/StockNemo">StockNemo<a>, for enormous help in implementing NNUE.

Below are some really nice people who has helped me during the development of Rice.
* <a href="https://github.com/pgg106">Zuppa</a>, author of <a href="https://github.com/PGG106/Alexandria/">Alexandria</a>.
* <a href="https://github.com/raklaptudirm">Rak</a>, author of <a href="https://github.com/raklaptudirm/mess">Mess</a>.
* <a href="https://github.com/archishou">Archi</a>, author of <a href="https://github.com/archishou/MidnightChessEngine">MidnightChessEngine<a>.
* <a href="https://github.com/Ciekce">Ciekce/a>, author of <a href="https://github.com/Ciekce/Polaris">Polaris<a>.

* and many more.