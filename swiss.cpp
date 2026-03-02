#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <map>
#include <limits>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

// Forward declarations
class Player;
struct Match;
struct TournamentState;

// Enum for player status and color history
enum class PlayerStatus
{
    ACTIVE,
    WITHDRAWN
};
enum class Color
{
    NONE,
    WHITE,
    BLACK
};

// ==========================================================
//            CLASS AND STRUCT DEFINITIONS
// ==========================================================
struct TournamentState
{
    vector<Player> players;
    int start_round = 1;
};

class Player
{
private:
    string id;
    string name;
    float score;
    int games_as_white;
    int games_played;
    vector<string> opponent_ids;
    float buchholz_score;
    bool has_received_bye;
    PlayerStatus status;
    vector<Color> color_history; // Full history for streak checking

public:
    Player(const string &player_id, const string &player_name);
    string getID() const;
    string getName() const;
    float getScore() const;
    float getBuchholzScore() const;
    const vector<string> &getOpponentIDs() const;
    const vector<Color> &getColorHistory() const;
    bool hasReceivedBye() const;
    PlayerStatus getStatus() const;
    int getColorDifference() const;
    int getGamesPlayed() const;
    int getGamesAsWhite() const;
    Color getLastColor() const;
    int getColorStreak(Color c) const;
    void addGameResult(float points, bool was_white);
    void addOpponent(const string &opponent_id);
    void setBuchholz(float score);
    void setReceivedBye();
    void setStatus(PlayerStatus new_status);
    bool hasPlayedAgainst(const string &opponent_id) const;
    void setState(float new_score, int new_games_played, int new_games_as_white, bool had_bye, PlayerStatus new_status);
    void setOpponents(const vector<string> &new_opponents);
    void setColorHistory(const vector<Color> &history);
};

struct Match
{
    string white_player_id;
    string black_player_id;
};

// ==========================================================
//            FUNCTION DECLARATIONS
// ==========================================================
bool addPlayer(vector<Player> &players, int serial_num);
void withdrawPlayer(vector<Player> &players);
bool loadPlayersFromFile(const string &filename, vector<Player> &players);
vector<Match> generatePairings(vector<Player> &players, int round);
vector<float> getUserResults(const vector<Match> &matches, const vector<Player> &players);
void processResults(vector<Player> &players, const vector<Match> &matches, const vector<float> &results);
void calculateBuchholzScores(vector<Player> &players);
void printStandings(vector<Player> &players, int round, ostream &out);
void saveTournament(const string &filename, const vector<Player> &players, int next_round);
TournamentState loadTournament(const string &filename);

// ==========================================================
//            MAIN FUNCTION
// ==========================================================
int main()
{
    vector<Player> players;
    string filename = "tournament_save.txt";
    int current_round = 1;
    bool end_tournament = false;

    cout << "Load existing tournament? (y/n): ";
    char choice;
    cin >> choice;

    if (choice == 'y' || choice == 'Y')
    {
        TournamentState loaded_state = loadTournament(filename);
        players = loaded_state.players;
        current_round = loaded_state.start_round;
    }

    // if (players.empty()) {
    //  cout << "--- New Tournament Roster Creation ---" << endl;
    //  int serial_counter = 1;
    //  while (addPlayer(players, serial_counter)) {
    //      serial_counter++;
    //  }

    if (players.empty())
    {
        cout << "--- New Tournament Roster Creation ---" << endl;
        cout << "Load roster from a text file? (y/n): ";
        char load_choice;
        cin >> load_choice;

        if (load_choice == 'y' || load_choice == 'Y')
        {
            string input_filename;
            cout << "Enter the filename (e.g., players.txt): ";
            cin >> input_filename;
            loadPlayersFromFile(input_filename, players);
        }
        else
        {
            int serial_counter = 1;
            while (addPlayer(players, serial_counter))
            {
                serial_counter++;
            }
        }

        if (players.size() < 2)
        {
            cout << "Error: You need at least 2 players to start. Exiting." << endl;
            return 1;
        }
        else
        {
            cout << "\n==============================================" << endl;
            cout << "Roster complete with " << players.size() << " players." << endl;
            cout << "==============================================" << endl;
        }
    }

    // --- Main Tournament Loop ---
    while (true)
    {
        cout << "\n==================== ROUND " << current_round << " ====================" << endl;
        while (true)
        {
            cout << "\nMake roster changes? (add/remove/end/continue): ";
            string change_choice;
            cin >> change_choice;
            if (change_choice == "add")
                addPlayer(players, players.size() + 1);
            else if (change_choice == "remove")
                withdrawPlayer(players);
            else if (change_choice == "end")
            {
                end_tournament = true;
                break;
            }
            else if (change_choice == "continue")
                break;
            else
                cout << "Invalid choice." << endl;
        }

        if (end_tournament)
            break;

        vector<Match> matches = generatePairings(players, current_round);

        long active_count = 0;
        for (const auto &p : players)
            if (p.getStatus() == PlayerStatus::ACTIVE)
                active_count++;
        if (active_count < 2)
        {
            cout << "Not enough active players to continue. Ending tournament." << endl;
            break;
        }

        if (!matches.empty())
        {
            vector<float> results = getUserResults(matches, players);
            processResults(players, matches, results);
        }

        calculateBuchholzScores(players);
        printStandings(players, current_round, cout);

        cout << "\nSave progress after this round? (y/n): ";
        cin >> choice;
        if (choice == 'y' || choice == 'Y')
        {
            saveTournament(filename, players, current_round + 1);
        }
        current_round++;
    }

    cout << "\n--- FINAL TOURNAMENT STANDINGS ---" << endl;
    printStandings(players, current_round - 1, cout);

    string results_filename;
    cout << "\nEnter a filename to save the final standings (e.g., results.txt): ";
    cin >> results_filename;
    ofstream results_file(results_filename);
    if (results_file.is_open())
    {
        printStandings(players, current_round - 1, results_file);
        results_file.close();
        cout << "Final standings have been saved to " << results_filename << endl;
    }
    else
    {
        cout << "Error: Could not open file to save standings." << endl;
    }

    return 0;
}

// ==========================================================
//            CLASS IMPLEMENTATION
// ==========================================================
Player::Player(const string &player_id, const string &player_name)
    : id(player_id), name(player_name), score(0.0f), games_as_white(0),
      games_played(0), buchholz_score(0.0f), has_received_bye(false),
      status(PlayerStatus::ACTIVE) {}

string Player::getID() const { return id; }
string Player::getName() const { return name; }
float Player::getScore() const { return score; }
float Player::getBuchholzScore() const { return buchholz_score; }
const vector<string> &Player::getOpponentIDs() const { return opponent_ids; }
const vector<Color> &Player::getColorHistory() const { return color_history; }
bool Player::hasReceivedBye() const { return has_received_bye; }
PlayerStatus Player::getStatus() const { return status; }
int Player::getGamesPlayed() const { return games_played; }
int Player::getGamesAsWhite() const { return games_as_white; }
Color Player::getLastColor() const { return color_history.empty() ? Color::NONE : color_history.back(); }
int Player::getColorDifference() const
{
    int games_as_black = games_played - games_as_white;
    return games_as_white - games_as_black;
}
int Player::getColorStreak(Color c) const
{
    int streak = 0;
    for (int i = color_history.size() - 1; i >= 0; --i)
    {
        if (color_history[i] == c)
            streak++;
        else
            break;
    }
    return streak;
}
void Player::addGameResult(float points, bool was_white)
{
    score += points;
    games_played++;
    if (was_white)
    {
        games_as_white++;
        color_history.push_back(Color::WHITE);
    }
    else
    {
        color_history.push_back(Color::BLACK);
    }
}
void Player::addOpponent(const string &opponent_id) { opponent_ids.push_back(opponent_id); }
void Player::setBuchholz(float new_score) { buchholz_score = new_score; }
void Player::setReceivedBye() { has_received_bye = true; }
void Player::setStatus(PlayerStatus new_status) { status = new_status; }
bool Player::hasPlayedAgainst(const string &opponent_id) const
{
    for (const string &past_id : opponent_ids)
        if (past_id == opponent_id)
            return true;
    return false;
}
void Player::setState(float new_score, int new_games_played, int new_games_as_white, bool had_bye, PlayerStatus new_status)
{
    score = new_score;
    games_played = new_games_played;
    games_as_white = new_games_as_white;
    has_received_bye = had_bye;
    status = new_status;
}
void Player::setOpponents(const vector<string> &new_opponents) { opponent_ids = new_opponents; }
void Player::setColorHistory(const vector<Color> &history) { color_history = history; }

// ==========================================================
//            FUNCTION IMPLEMENTATIONS
// ==========================================================
// bool addPlayer(vector<Player>& players, int serial_num) { /* ... implementation from previous response ... */ }
bool addPlayer(vector<Player> &players, int serial_num)
{
    string name, id;
    cout << "\n--- Player #" << serial_num << " ---" << endl;
    cout << "Enter player's name (or type 'done' to finish): ";
    if (cin.peek() == '\n')
        cin.ignore();
    getline(cin, name);
    if (name == "done")
        return false;
    cout << "Enter player's unique ID for " << name << ": ";
    cin >> id;
    for (const auto &p : players)
    {
        if (p.getID() == id)
        {
            cout << "Error: A player with this ID already exists." << endl;
            return true;
        }
    }
    players.push_back(Player(id, name));
    return true;
}
// void withdrawPlayer(vector<Player>& players) { /* ... implementation from previous response ... */ }
void withdrawPlayer(vector<Player> &players)
{
    string name;
    cout << "Enter the name of the player to withdraw: ";
    if (cin.peek() == '\n')
        cin.ignore();
    getline(cin, name);
    vector<Player *> matches;
    for (auto &p : players)
    {
        if (p.getName() == name && p.getStatus() == PlayerStatus::ACTIVE)
        {
            matches.push_back(&p);
        }
    }
    if (matches.empty())
    {
        cout << "No active player with that name found." << endl;
    }
    else if (matches.size() == 1)
    {
        matches[0]->setStatus(PlayerStatus::WITHDRAWN);
        cout << matches[0]->getName() << " has been withdrawn from pairings." << endl;
    }
    else
    {
        cout << "Multiple active players found. Please choose by ID:" << endl;
        for (const auto *p : matches)
            cout << "- " << p->getName() << " (ID: " << p->getID() << ")" << endl;
        string id_to_withdraw;
        cout << "Enter the ID of the player to withdraw: ";
        cin >> id_to_withdraw;
        bool success = false;
        for (auto *p : matches)
        {
            if (p->getID() == id_to_withdraw)
            {
                p->setStatus(PlayerStatus::WITHDRAWN);
                cout << p->getName() << " (ID: " << p->getID() << ") has been withdrawn." << endl;
                success = true;
                break;
            }
        }
        if (!success)
            cout << "Invalid ID. No player withdrawn." << endl;
    }
}

vector<Match> generatePairings(vector<Player> &players, int round)
{
    vector<Player *> active_players;
    for (auto &p : players)
        if (p.getStatus() == PlayerStatus::ACTIVE)
            active_players.push_back(&p);
    cout << "\nThere are " << active_players.size() << " active players for this round." << endl;

    if (round == 1)
    {
        cout << "--- Applying Random Pairing for Round 1 ---" << endl;
        random_device rd;
        mt19937 g(rd());
        shuffle(active_players.begin(), active_players.end(), g);
    }
    else
    {
        sort(active_players.begin(), active_players.end(), [](const Player *a, const Player *b)
             {
            if (a->getScore() != b->getScore()) return a->getScore() > b->getScore();
            if (a->getBuchholzScore() != b->getBuchholzScore()) return a->getBuchholzScore() > b->getBuchholzScore();
            return a->getID() < b->getID(); });
        // sort(active_players.begin(), active_players.end(), [](const Player& a, const Player& b) {
        //     if (a.getScore() != b.getScore()) return a.getScore() > b.getScore();
        //     if (a.getBuchholzScore() != b.getBuchholzScore()) return a.getBuchholzScore() > b.getBuchholzScore();
        //     return a.getID() < b.getID();
        // });
    }

    vector<Match> round_matches;
    vector<bool> is_paired(active_players.size(), false);

    if (active_players.size() % 2 != 0)
    {
        for (int i = active_players.size() - 1; i >= 0; --i)
        {
            if (!active_players[i]->hasReceivedBye())
            {
                for (auto &p_main : players)
                {
                    if (p_main.getID() == active_players[i]->getID())
                    {
                        cout << "\n--- Bye ---" << endl;
                        cout << p_main.getName() << " receives the bye." << endl;
                        p_main.setReceivedBye();
                        p_main.addGameResult(1.0f, false);
                        break;
                    }
                }
                is_paired[i] = true;
                break;
            }
        }
    }

    auto pair_players = [&](Player *player1, Player *player2)
    {
        // --- Step 1: Enforce absolute streak rule ---
        if (player1->getColorStreak(Color::WHITE) >= 2)
        {
            round_matches.push_back({player2->getID(), player1->getID()});
            return;
        }
        if (player1->getColorStreak(Color::BLACK) >= 2)
        {
            round_matches.push_back({player1->getID(), player2->getID()});
            return;
        }
        if (player2->getColorStreak(Color::WHITE) >= 2)
        {
            round_matches.push_back({player1->getID(), player2->getID()});
            return;
        }
        if (player2->getColorStreak(Color::BLACK) >= 2)
        {
            round_matches.push_back({player2->getID(), player1->getID()});
            return;
        }

        // --- Step 2: If no constraints, determine preference ---
        int diff1 = player1->getColorDifference();
        int diff2 = player2->getColorDifference();
        bool p1_gets_white_preference;

        if (diff1 < diff2)
        {
            p1_gets_white_preference = true;
        }
        else if (diff2 < diff1)
        {
            p1_gets_white_preference = false;
        }
        else
        {
            if (player1->getLastColor() == Color::BLACK && player2->getLastColor() != Color::BLACK)
            {
                p1_gets_white_preference = true;
            }
            else if (player2->getLastColor() == Color::BLACK && player1->getLastColor() != Color::BLACK)
            {
                p1_gets_white_preference = false;
            }
            else
            {
                p1_gets_white_preference = true;
            }
        }

        if (p1_gets_white_preference)
        {
            round_matches.push_back({player1->getID(), player2->getID()});
        }
        else
        {
            round_matches.push_back({player2->getID(), player1->getID()});
        }
    };

    for (size_t i = 0; i < active_players.size(); ++i)
    {
        if (is_paired[i])
            continue;
        for (size_t j = i + 1; j < active_players.size(); ++j)
        {
            if (is_paired[j])
                continue;
            if (!active_players[i]->hasPlayedAgainst(active_players[j]->getID()))
            {
                pair_players(active_players[i], active_players[j]);
                is_paired[i] = true;
                is_paired[j] = true;
                break;
            }
        }
    }

    for (size_t i = 0; i < active_players.size(); ++i)
    {
        if (is_paired[i])
            continue;
        for (size_t j = i + 1; j < active_players.size(); ++j)
        {
            if (is_paired[j])
                continue;
            cout << "Forcing a rematch to complete pairings: " << active_players[i]->getName() << " vs " << active_players[j]->getName() << endl;
            pair_players(active_players[i], active_players[j]);
            is_paired[i] = true;
            is_paired[j] = true;
            break;
        }
    }
    // 3. NEW: Assign the bye to the single player left unpaired at the end
    // if (active_players.size() % 2 != 0) {
    //     for (size_t i = 0; i < active_players.size(); ++i) {
    //         if (!is_paired[i]) { // Find the one player who was not paired
    //             for (auto& p_main : players) {
    //                 if (p_main.getID() == active_players[i].getID()) {
    //                      cout << "\n--- Bye ---" << endl;
    //                      cout << p_main.getName() << " receives the bye." << endl;
    //                      p_main.setReceivedBye();
    //                      p_main.addGameResult(1.0f, false);
    //                      break;
    //                 }
    //             }
    //             break;
    //         }
    //     }
    // }
    return round_matches;
}

// vector<float> getUserResults(const vector<Match>& matches, const vector<Player>& players) { /* ... implementation from previous response ... */ }
// void processResults(vector<Player>& players, const vector<Match>& matches, const vector<float>& results) { /* ... implementation from previous response ... */ }
// void calculateBuchholzScores(vector<Player>& players) { /* ... implementation from previous response ... */ }
// void printStandings(vector<Player>& players, int round, ostream& out) { /* ... implementation from previous response ... */ }
vector<float> getUserResults(const vector<Match> &matches, const vector<Player> &players)
{
    cout << "\n--- Enter Match Results ---" << endl;
    cout << "(1=White win, 0=Black win, 0.5=Draw)" << endl;
    map<string, string> name_map;
    for (const auto &p : players)
        name_map[p.getID()] = p.getName();
    vector<float> results(matches.size(), -1.0f);
    int results_entered = 0;
    while (true)
    {
        cout << "\n-- Current Pairings --" << endl;
        for (size_t i = 0; i < matches.size(); ++i)
        {
            cout << i + 1 << ". " << name_map.at(matches[i].white_player_id) << " (W) vs. "
                 << name_map.at(matches[i].black_player_id) << " (B)";
            if (results[i] != -1.0f)
            {
                cout << " [Result: ";
                if (results[i] == 1.0f)
                    cout << "White Wins";
                else if (results[i] == 0.0f)
                    cout << "Black Wins";
                else
                    cout << "Draw";
                cout << "]" << endl;
            }
            else
            {
                cout << " [Pending]" << endl;
            }
        }

        if (results_entered == matches.size())
        {
            cout << "\nAll results entered. Submit round results? (y/n): ";
            char submit_choice;
            cin >> submit_choice;
            if (submit_choice == 'y' || submit_choice == 'Y')
            {
                break; // This is what finally ends the loop
            }
            // If they type 'n', the loop continues, allowing them to overwrite a mistake
        }

        int match_num;
        cout << "\nEnter match number to submit result: ";
        cin >> match_num;
        if (cin.fail() || match_num < 1 || match_num > matches.size())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid match number." << endl;
            continue;
        }
        size_t match_index = match_num - 1;
        bool is_new_result = (results[match_index] == -1.0f);
        if (!is_new_result)
        {
            cout << "A result has already been entered. Overwrite? (y/n): ";
            char overwrite_choice;
            cin >> overwrite_choice;
            if (overwrite_choice != 'y' && overwrite_choice != 'Y')
                continue;
        }
        float result;
        while (true)
        {
            cout << "Enter result for match " << match_num << ": ";
            cin >> result;
            if (cin.fail() || (result != 1.0f && result != 0.0f && result != 0.5f))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter 1, 0, or 0.5." << endl;
            }
            else
                break;
        }
        results[match_index] = result;
        if (is_new_result)
            results_entered++;
    }
    cout << "\nAll results for the round have been entered." << endl;
    return results;
}

void processResults(vector<Player> &players, const vector<Match> &matches, const vector<float> &results)
{
    for (size_t i = 0; i < matches.size(); ++i)
    {
        string white_id = matches[i].white_player_id;
        string black_id = matches[i].black_player_id;
        Player *white_player = nullptr;
        Player *black_player = nullptr;
        for (auto &p : players)
        {
            if (p.getID() == white_id)
                white_player = &p;
            if (p.getID() == black_id)
                black_player = &p;
        }
        if (white_player && black_player)
        {
            white_player->addOpponent(black_id);
            black_player->addOpponent(white_id);
            float white_score = results[i], black_score = 1.0f - white_score;
            white_player->addGameResult(white_score, true);
            black_player->addGameResult(black_score, false);
        }
    }
}

void calculateBuchholzScores(vector<Player> &players)
{
    map<string, float> score_map;
    for (const auto &p : players)
        score_map[p.getID()] = p.getScore();
    for (auto &p : players)
    {
        vector<float> opponent_scores;
        for (const string &opponent_id : p.getOpponentIDs())
        {
            if (score_map.count(opponent_id))
            {
                opponent_scores.push_back(score_map.at(opponent_id));
            }
        }
        float median_buchholz_total = 0;
        sort(opponent_scores.begin(), opponent_scores.end());
        if (opponent_scores.size() > 2)
        {
            for (size_t i = 1; i < opponent_scores.size() - 1; ++i)
            {
                median_buchholz_total += opponent_scores[i];
            }
        }
        else
        {
            for (float score : opponent_scores)
                median_buchholz_total += score;
        }
        p.setBuchholz(median_buchholz_total);
    }
}

void printStandings(vector<Player> &players, int round, ostream &out)
{
    sort(players.begin(), players.end(), [](const Player &a, const Player &b)
         {
                if (a.getScore() != b.getScore()) return a.getScore() > b.getScore();
                if (a.getBuchholzScore() != b.getBuchholzScore()) return a.getBuchholzScore() > b.getBuchholzScore();
                return a.getID() < b.getID(); });
    out << "\n--- Standings after Round " << round << " ---" << endl;
    out << left << setw(5) << "Rank" << setw(20) << "Name"
        << setw(10) << "Score" << setw(12) << "Buchholz" << setw(12) << "Status"
        << setw(15) << "ID" << endl;
    out << "--------------------------------------------------------------------------------" << endl;
    int last_rank = 0;
    for (size_t i = 0; i < players.size(); ++i)
    {
        int current_rank;
        if (i > 0 &&
            players[i].getScore() == players[i - 1].getScore() &&
            players[i].getBuchholzScore() == players[i - 1].getBuchholzScore())
        {
            current_rank = last_rank;
        }
        else
        {
            current_rank = i + 1;
        }
        out << left << setw(5) << to_string(current_rank) + "."
            << setw(20) << players[i].getName()
            << setw(10) << fixed << setprecision(1) << players[i].getScore()
            << setw(12) << fixed << setprecision(1) << players[i].getBuchholzScore();
        if (players[i].getStatus() == PlayerStatus::WITHDRAWN)
        {
            out << setw(12) << "Withdrawn";
        }
        else
        {
            out << setw(12) << "Active";
        }
        out << setw(15) << players[i].getID() << endl;
        last_rank = current_rank;
    }
}
void saveTournament(const string &filename, const vector<Player> &players, int next_round)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Error saving file." << endl;
        return;
    }
    file << next_round << "\n";
    for (const auto &p : players)
    {
        file << p.getID() << "|" << p.getName() << "|" << p.getScore() << "|"
             << p.getGamesPlayed() << "|" << p.getGamesAsWhite() << "|" << p.hasReceivedBye() << "|"
             << static_cast<int>(p.getStatus()) << "|";
        for (Color c : p.getColorHistory())
            file << static_cast<int>(c) << " ";
        file << "|";
        for (const string &id : p.getOpponentIDs())
            file << id << " ";
        file << "\n";
    }
    file.close();
    cout << "Tournament progress saved to " << filename << endl;
}

TournamentState loadTournament(const string &filename)
{
    TournamentState state;
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Could not open save file." << endl;
        return state;
    }
    string line;
    if (getline(file, line) && !line.empty())
    {
        try
        {
            state.start_round = stoi(line);
        }
        catch (const std::exception &)
        {
            cout << "Warning: Old save file. Starting from Round 1." << endl;
            state.start_round = 1;
            file.clear();
            file.seekg(0, ios::beg);
        }
    }
    while (getline(file, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string item;
        getline(ss, item, '|');
        string id = item;
        getline(ss, item, '|');
        string name = item;
        getline(ss, item, '|');
        float score = stof(item);
        getline(ss, item, '|');
        int games = stoi(item);
        getline(ss, item, '|');
        int white_games = stoi(item);
        getline(ss, item, '|');
        bool had_bye = stoi(item);
        getline(ss, item, '|');
        PlayerStatus status = static_cast<PlayerStatus>(stoi(item));

        Player p(id, name);
        p.setState(score, games, white_games, had_bye, status);

        getline(ss, item, '|');
        stringstream color_ss(item);
        vector<Color> history;
        int c_int;
        while (color_ss >> c_int)
            history.push_back(static_cast<Color>(c_int));
        p.setColorHistory(history);

        getline(ss, item, '|');
        stringstream opponent_ss(item);
        vector<string> opponent_ids;
        string opp_id;
        while (opponent_ss >> opp_id)
            opponent_ids.push_back(opp_id);
        p.setOpponents(opponent_ids);

        state.players.push_back(p);
    }
    if (!state.players.empty())
        cout << "Tournament progress loaded from " << filename << endl;
    return state;
}

bool loadPlayersFromFile(const string &filename, vector<Player> &players)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Error: Could not open file '" << filename << "'." << endl;
        return false;
    }

    string line;
    int count = 0;
    while (getline(file, line))
    {
        if (line.empty())
            continue;

        stringstream ss(line);
        string id, name;

        // Read up to the comma for the ID, then the rest of the line for the Name
        if (getline(ss, id, ',') && getline(ss, name))
        {
            // Check to ensure we don't add duplicate IDs
            bool duplicate = false;
            for (const auto &p : players)
            {
                if (p.getID() == id)
                {
                    cout << "Warning: Skipping duplicate ID " << id << endl;
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
            {
                players.push_back(Player(id, name));
                count++;
            }
        }
    }
    file.close();
    cout << "Successfully loaded " << count << " players from " << filename << endl;
    return true;
}
