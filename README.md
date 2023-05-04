# Rice

<div align="center">
    <img src="./imgs/logo.png" alt="Rice logo by Midjourney" width="30%">
    <br>
    <br>
    <b><i>strong, free and open source UCI chess engine written in C++.</i></b>
    <br>
    <br>
    <img src="https://img.shields.io/github/downloads/rafid-dev/rice/total?color=critical&style=for-the-badge">
    <img src="https://img.shields.io/github/license/rafid-dev/rice?color=blue&style=for-the-badge">
    <br>
    <img src="https://img.shields.io/github/v/release/rafid-dev/rice?color=blue&label=Latest%20release&style=for-the-badge">
    <img src="https://img.shields.io/github/last-commit/rafid-dev/rice?color=critical&style=for-the-badge">
</div>

## Features

### Board Representation
* Bitboards
* Fancy magics for movegen

### Search

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
* Search Extensions

### Evaluation
* NNUE (Efficiently updateable neural network)
* Net Architecture: 768->256x2->1

## Building

Clang is preferred.

```bash
$ git clone https://github.com/rafid-dev/rice
$ cd rice/src
$ make 
$ ./Rice
```

### Usage
The Universal Chess Interface (UCI) is a standard protocol used to communicate with
a chess engine, and is the recommended way to do so for typical graphical user interfaces
(GUI) or chess tools. Rice requires a <a href="https://www.chessprogramming.org/UCI#GUIs">UCI-compatible graphical user interface</a> in order to be used with the protocol.

### NNUE
From 5.0, Rice has switched to NNUE from its handcrafted evaluation.

When I lack the hardware resources to generate enough training data, I need to use data generated by external engines. However, I think it's important to be transparent about the use of these engines and where the training data comes from.

## <a href="https://github.com/cosmobobak/viridithas/"> Viridithas by Cosmo
Rice 5.0 has used data generated by Viridithas, a strong chess engine by Cosmo.

## <a href="https://lczero.org"> Leela Chess Zero </a> by the Lc0 Team
LC0 employs a novel method for playing high-level chess by utilizing MCTS and acquiring its chess knowledge through self-play. As Rice 6.0 utilizes data from LC0, it is worth noting that this data is licensed under the Open Database License, which promotes the sharing and reuse of data while maintaining legal protection for contributors and users.

### Acknowledgements

* <a href="https://github.com/Disservin">Disservin</a>, author of <a href="https://github.com/Disservin/Smallbrain">Smallbrain</a> for his <a href="https://github.com/Disservin/chess-library">chess library in c++</a> which is used in Rice.

* <a href="https://github.com/TheBlackPlague">Shaheryar Sohail</a>, author of <a href="https://github.com/TheBlackPlague/StockNemo">StockNemo<a>, for enormous help in implementing NNUE and for MantaRay.

* <a href="https://github.com/pgg106">Zuppa</a>, author of <a href="https://github.com/PGG106/Alexandria/">Alexandria</a>.
* <a href="https://github.com/raklaptudirm">Rak</a>, author of <a href="https://github.com/raklaptudirm/mess">Mess</a>.
* <a href="https://github.com/archishou">Archi</a>, author of <a href="https://github.com/archishou/MidnightChessEngine">MidnightChessEngine<a>.
* <a href="https://github.com/Ciekce">Ciekce, author of <a href="https://github.com/Ciekce/Polaris">Polaris<a>.

* <a href="https://github.com/dsekercioglu/marlinflow">marlinflow</a> for Rice's current neural network trainer.