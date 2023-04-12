# Rice

<div align="center">
  <kbd>
    <img src="./imgs/logo.png" />
  </kbd>
</div>

## Description

Rice is a strong, free and open source UCI chess engine written in C++.

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
* Search Extensions

# Evaluation
* NNUE (Efficiently updateable neural network)
* Net Architecture: 768->256x2->1

### Building

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

### Acknowledgements

I would specially like to thank <a href="https://github.com/Disservin">Disservin</a>, author of <a href="https://github.com/Disservin/Smallbrain">Smallbrain</a> for his <a href="https://github.com/Disservin/chess-library">chess library in c++</a> which is used in Rice.

I would also like to specially thank <a href="https://github.com/TheBlackPlague">Shaheryar Sohail</a>, author of <a href="https://github.com/TheBlackPlague/StockNemo">StockNemo<a>, for enormous help in implementing NNUE.

Below are some really nice people who has helped me during the development of Rice.
* <a href="https://github.com/pgg106">Zuppa</a>, author of <a href="https://github.com/PGG106/Alexandria/">Alexandria</a>.
* <a href="https://github.com/raklaptudirm">Rak</a>, author of <a href="https://github.com/raklaptudirm/mess">Mess</a>.
* <a href="https://github.com/archishou">Archi</a>, author of <a href="https://github.com/archishou/MidnightChessEngine">MidnightChessEngine<a>.
* <a href="https://github.com/Ciekce">Ciekce/a>, author of <a href="https://github.com/Ciekce/Polaris">Polaris<a>.

* and many more.

### License

This project is licensed under the [MIT License](LICENSE).