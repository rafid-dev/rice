<div align="center">
  <a href=".">
    <img src="./imgs/logo.png" height="200"/>
  </a>
</div>
[![MIT License][license-shield]][license-url]


# Overview

Rice is a free and open source UCI chess engine written in C.

Rice is not a complete chess program and requires a <a href="https://www.chessprogramming.org/UCI#GUIs">UCI-compatible graphical user interface</a> in order to be used comfortably.

### Features
# Board Representation
* Bitboards

# Search

* Negamax
* Alpha beta pruning
* Quiescence search
* PVS (Principal Variation Search)
* ZWS (Zero Window Search)
* LMR (Late Move Reduction)
* SEE Pruning (Static Exchange Evaluation Pruning)
* Movecount Pruning/LMP (Late Move Pruning)
* Transposition Table cutoffs and move ordering
* History, killers and MVVLVA Move ordering

# Evaluation (HCE)
* PeSTO Tables
* Rook open file bonus
* and much more to add

# Acknowledgements

I would specially like to thank <a href="https://github.com/Disservin">Disservin</a>, author of <a href="https://github.com/Disservin/Smallbrain">Smallbrain</a> for his <a href="https://github.com/Disservin/chess-library">chess library in c++</a> which is used in Rice.

Below are some really nice people who has helped me during the development of Rice.
* <a href="https://github.com/pgg106">Zuppa</a>, author of <a href="https://github.com/PGG106/Alexandria/">Alexandria</a>.
* <a href="https://github.com/raklaptudirm">Rak</a>, author of <a href="https://github.com/raklaptudirm/mess">Mess</a>.
* <a href="https://github.com/AndyGrant">Andy Grant</a>, author of <a href="https://github.com/AndyGrant/Ethereal/">Ethereal</a>

* and many more.