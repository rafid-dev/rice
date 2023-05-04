#include "datagen.h"

std::atomic<bool> stop_flag(false);
std::atomic<uint64_t> total_fens(0);
std::atomic<uint64_t> total_games(0);

static inline std::string ToString(FenData &fenData)
{
    return fenData.fen + " | " + std::to_string(fenData.eval) + " | " + fenData.wdl;
}

static inline void NodesOver(SearchInfo &info)
{
    if (info.nodeset && info.nodes >= info.stopNodes)
    {
        info.stopped = true;
    }
}

 inline void MakeRandomMoves(Board &board)
{
    srand(time(NULL));
    Movelist list;

    Movegen::legalmoves<ALL>(board, list);

    if (list.size == 0)
    {
        return;
    }

    Move randomMove = list.list[rand() % list.size].move;
    board.makeMove(randomMove);
    return;
}

static inline void SetNewGameState(Board &board, SearchInfo &info)
{
    ClearForSearch(info, table);

    board.applyFen(DEFAULT_POS);
}

int search_best_move(Board &board, SearchInfo &info)
{
    SearchStack stack[MAXPLY + 10];
    SearchStack *ss = stack + 7; // Have some safety overhead.

    ClearForSearch(info, table);
    
    board.Refresh();

    int score = 0;

    for (int i = 1; i <= info.depth; i++)
    {
        score = AlphaBeta(-INF_BOUND, INF_BOUND, i, board, info, ss);
        NodesOver(info);
        if (info.stopped)
        {
            break;
        }
    }
    
    return score;
}

bool game_over(Board &board, SearchInfo &info, std::string &wdl, int ply)
{

    if (board.isRepetition()){
        wdl = "0.5";
        return true;
    }

    Movelist moves;
    Movegen::legalmoves<ALL>(board, moves);

    bool InCheck =
        board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));

    if (moves.size == 0)
    {
        if (!InCheck)
        {
            wdl = "0.5";
            return true;
        }
        else
        {
            wdl = board.sideToMove == White ? "0.0" : "1.0";
            return true;
        }
    }
    if (ply > 1500)
    {
        wdl = "0.5";
        return true;
    }
    return false;
}

int sanity_search(Board &board, SearchInfo &info)
{
    SearchStack stack[MAXPLY + 10];
    SearchStack *ss = stack + 7; // Have some safety overhead.

    ClearForSearch(info, table);
    board.Refresh();

    int score = 0;

    score = AlphaBeta(-INF_BOUND, INF_BOUND, 10, board, info, ss);


    return score;
}

bool play_game(Board &board, SearchInfo &info, std::ofstream &outfile)
{
    SetNewGameState(board, info);

    std::string wdl;

    for (int i = 0; i < 6; i++)
    {
        MakeRandomMoves(board);
    }

    if (abs(sanity_search(board, info)) > 1000){
        return false;
    }

    int ply = 6;

    std::vector<FenData> entries;

    while(!game_over(board, info, wdl, ply)){

        FenData entry;
        entry.fen = board.getFen();

        int score =  search_best_move(board, info);
        entry.eval = board.sideToMove == White ? score : -score;

        Move move = info.bestmove;

        bool cap = is_capture(board, move);

        bool InCheck =
            board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
        //std::cout << convertMoveToUci(info.pv_table.array[0][0]) << std::endl;
        board.makeMove(info.bestmove);
        ply++;

        bool TheirCheck =
            board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));

        if (!(ply < 8 || TheirCheck || InCheck || abs(entry.eval) > ISMATE || cap)){
            entries.push_back(entry);   
        }
    }

    total_fens += entries.size();
    total_games++;

    for (FenData& entry : entries)
    {
        entry.wdl = wdl;
        outfile << ToString(entry) << "\n";
    }


    return true;
}

void playGames(int id, int games , int threadcount)
{
    Board board;
    SearchInfo info;
    info.timeset = false;
    info.stopNodes = 5000;
    info.nodeset = true;
    info.depth = MAXDEPTH;

    std::ofstream outfile("./data" + std::to_string(id) + ".txt", std::ios::app);
    
    auto start_time = GetTimeMs();

    if (outfile.is_open())
    {
        if (id == 0){
            std::cout << "datagen started successfully! with " << threadcount << " threads and " << games << " games!" << std::endl;
        }
        for (int i = 1; i <= games; i++)
        {
            if (stop_flag){
                if (id == 0){
                    std::cout << "stopping datagen" << std::endl;
                }
                break;
            }
            
            //std::cout << "Game " << i << std::endl;
            if (!play_game(board, info, outfile))
            {
                i--;
                continue;
            }

            if (id == 0 && !(i % 100)){
                std::cout << "gamecount "<< i << ":\t| total games: " << total_games << " | total_fens: " << total_fens << " | speed: " << (total_fens * 1000 / (1 + GetTimeMs() - start_time)) << " fens/s | " << ((total_fens * 1000 / (1 + GetTimeMs() - start_time))*60) << " fens/m | " << ((total_fens * 1000 / (1 + GetTimeMs() - start_time))*60*60) << " fens/hour" << std::endl;
            }
        }
    }else std::cout << "Unable to open file" << std::endl;

    std::cout << "Thread " << id << "Finished" << std::endl;
    return;
}

void generateData(int games, int threadCount)
{
    //playGames(0, games, threadCount);
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; i++){
        threads.emplace_back(std::thread(playGames, i, games, threadCount));
    }

    for (auto& thread : threads){
        if (thread.joinable()){
            thread.join();
        }
    }

    std::cout << "Datagen done!\n";
}