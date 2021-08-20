//Nicholas Heerdt cs302 section 1002
//assignment 12
//Compile as: g++ ast12.cpp -g -lncurses -std=c++17

//#include<iostream>    //cin/cout NOT USED. DO NOT ENABLE!
#include<fstream>       //File Streams
#include<ncurses.h>     //N Curses Library
#include<stdlib.h>      //srand
#include<time.h>        //time
#include<string>
#include<vector>
#include<queue>
#include<stack>
#include<climits>

using namespace std;
void initCurses();
void endCurses();

const int ROUNDLEN = 300;


enum colision { None, Actor, Tunnel, Thin, Full};//player is like thin except it is invisible to pathfinding
enum tunnel_prop { Non, Horizontal, Vertical};
enum dire {noned, self, lef, righ, up, down};
enum ai_mode {search_and_destroy, search, defend_bomb, recover_bomb, defuse_bomb, combat, escape};
enum ent_type {none, bullet, bomb, terrorist, counter, player};
enum instruction {move_u, move_d, move_l, move_r, idle, plant_b, defuse_b, shoot};
enum victory {inprog, Terroist_vic, Counter_vic};

//for future reference
//turn ordre
//1. player
//2. ai
//3 projectiles

class Level;

class ent_t {
public:

    bool has_bomb = false;
    bool isactor = false;
    char symbol;
    int x_coord;
    int y_coord;
    colision has_colision;
    bool blocks_view;
    bool is_terrorist;

    Level *level_ptr;

    ent_type type;

    //up    arr[--x][y]
    //down  arr[++x][y]
    //left  arr[x][--y]
    //right arr[x][++y]

    virtual void setCoordinates(int x, int y) {
        x_coord = x;
        y_coord = y;
        return;
    }
    virtual void tick() {
        return;   //what an entity does every tick
    }
    virtual void die() {
        return;   //when something
    }


};

class projectile_t: public ent_t { //bomb, bullets, and if I get to it grenades
public:
    int health;//more of a timer but this gets the point across
    int velocity; //bomb = 0, bullet 5?, grenade 2
    dire direction;
    tunnel_prop is_in_tunnel;
};

class bomb_i: public projectile_t {
public:
    bomb_i(int x, int y, Level *lvlptr);
    void tick();
};

class bomb_t: public projectile_t {
public:

    bool currdef = false;
    int defusetime = 5;
    Level *level_ptr;

    bomb_t(int x, int y, Level *lvlptr);

    void tick();

    void die() {

    }


};

class bullet_t: public projectile_t {
public:

    bool notdead = true;

    bullet_t(int x, int y, dire direc, bool ister, Level *lvl) {
        this->level_ptr = lvl;
        this->is_terrorist = ister;
        this->type = bullet;
        this->x_coord = x;
        this->y_coord = y;
        this->velocity = 1;
        this->health = 999;
        this->direction = direc;
        this->symbol = '*';

    }
    void tick();
    void die();
    void kill_enemies() {}

    ~bullet_t();


};

class actor: public ent_t {
public:

    bool idle = false;
    int health;
    dire direction;
    bool isai;
    tunnel_prop is_in_tunnel;

    int xspawn;
    int yspawn;
    bool isalive;

    stack<instruction> ins_stack;

    actor() {
        this->has_colision = Actor;
    }

    bool move_up();
    bool move_down();
    bool move_right();
    bool move_left();

    void respawn();

    void shoot();

    void die();
};

class terroist_t: public actor {
public:
    int site_target;

    terroist_t(int x, int y, Level *lvl) {
        this->isactor = true;
        this->isalive = true;
        this->level_ptr = lvl;
        this->is_in_tunnel = Non;
        this->type = terrorist;
        this->x_coord = x;
        this->y_coord = y;
        this->xspawn = x;
        this->yspawn = y;
        this->is_terrorist = true;
        this->symbol = 'T';

    }

    void tick();

};

class counter_t: public actor {
public:
    counter_t(int x, int y, Level *lvl) {
        this->isactor = true;
        this->isalive = true;
        this->xspawn = x;
        this->yspawn = y;
        this->is_in_tunnel = Non;
        this->level_ptr = lvl;
        this->type = counter;
        this->x_coord = x;
        this->y_coord = y;
        this->is_terrorist = false;
        this->symbol = 'C';
    }

    void tick();
};

class player_t: public actor {
public:


    player_t(int x, int y, bool ister, Level *lvl) {
        this->isactor = true;
        this->isalive = true;
        this->xspawn = x;
        this->yspawn = y;
        this->is_in_tunnel = Non;
        this->direction = up;
        this->level_ptr = lvl;
        this->type = player;
        this->x_coord = x;
        this->y_coord = y;
        this->is_terrorist = ister;
        this->symbol = '@';
    }


    bool ptick(char ins) {

        if (!(this->isalive)) {
            return true;
        }

        if (ins == 'w') {
            return this->move_up();
        }

        if (ins == 'a') {
            return this->move_left();
        }

        if (ins == 's') {
            return this->move_down();
        }

        if (ins == 'd') {
            return this->move_right();
        }

        if (ins == 'q') {
            return true;
        }

        if (ins == ' ') {
            this->shoot();
            return true;
        }

        if (ins == 'i') {
            return true;
        } else {
            return false;
        }

    }

};

class point_t {
public:
    int siteno = 0;
    int node_no;
    int x_coord;
    int y_coord;
    bool isbombsite = false;
    char symbol;
    colision has_colision;
    bool blocks_view;
    std::vector<ent_t *> entList;

    virtual void printP() {
        if (entList.empty()) {
            printw("%c", symbol);
            return;
        } else {
            char toprnt = '7';//7 is for debugging purposes
            ent_type heaviest = none;

            for (int i = 0; i < entList.size(); i++) {
                if (entList[i]->type > heaviest) {
                    heaviest = entList[i]->type;
                    toprnt = entList[i]->symbol;
                }
            }

            printw("%c", toprnt);

        }
    }

    ~point_t() {
        for (int i = 0; i < entList.size(); i++) {
            delete entList[i];
        }

    }

};

class tunnel_t: public point_t {
public:

    int secnode;
    tunnel_t(int x, int y, int node) {
        this->node_no = node;
        secnode = node + 1;
        this->symbol = '#';
        this->has_colision = Tunnel;
        this->blocks_view = false;
        this->x_coord = x;
        this->y_coord = y;

    }

};

class site_t: public point_t {
public:


    site_t() {
        this->isbombsite = true;
        this->has_colision = None;
        this->blocks_view = false;
    }

    site_t(int x, int y, int node, Level *lvlptr);
};

class site_1: public site_t {
public:
    site_1(int x, int y, int node) {
        this->node_no = node;
        this->siteno = 1;
        this->symbol = '1';
        this->x_coord = x;
        this->y_coord = y;
    }

};

class site_2: public site_t {
public:
    site_2(int x, int y, int node) {
        this->node_no = node;
        this->siteno = 2;
        this->symbol = '2';
        this->x_coord = x;
        this->y_coord = y;
    }

};

class site_3: public site_t {
public:
    site_3(int x, int y, int node) {
        this->node_no = node;
        this->siteno = 3;
        this->symbol = '3';
        this->x_coord = x;
        this->y_coord = y;
    }

};

class empty_t: public point_t {
public:
    empty_t(int x, int y, int node) {
        this->node_no = node;
        this->symbol = ' ';
        this->has_colision = None;
        this->blocks_view = false;
        this->x_coord = x;
        this->y_coord = y;
    }
};

class ct_spawn: public point_t {
public:
    void spawn() {
        //search local area for 5 spots
        //if player is not a terrorist spawn player and spawn 4 CTs
        //else spawn 5
    }

    ct_spawn(int x, int y, int node) {
        this->node_no = node;
        this->symbol = 'C';
        this->has_colision = None;
        this->blocks_view = false;
        this->x_coord = x;
        this->y_coord = y;

    }
};

class t_spawn: public point_t {
public:
    void spawn() {
        //search local area for 5 spots
        //if player is a terrorist spawn player and spawn 4 Ts
        //else spawn 5
    }

    t_spawn(int x, int y, int node) {
        this->node_no = node;
        this->symbol = 'T';
        this->has_colision = None;
        this->blocks_view = false;
        this->x_coord = x;
        this->y_coord = y;

    }
};


class wall_t: public point_t {
public:
    wall_t(int x, int y) {
        this->symbol = 'x';
        this->has_colision = Full;
        this->blocks_view = true;
        this->x_coord = x;
        this->y_coord = y;
    }
};

class obsticle_t: public point_t {
public:
    obsticle_t(int x, int y) {
        this->symbol = 'o';
        this->has_colision = Thin;
        this->blocks_view = false;
        this->x_coord = x;
        this->y_coord = y;
    }
};



class CharMap {
public:
    CharMap(char *arg);
    CharMap(char **c, string m, int w, int h) :
        map(c), mapName(m), width(w), height(w) {}
    ~CharMap();
    void print();
    char **map;
    string mapName;
    int width;
    int height;



};

class Level {
public:


    point_t ***point;

    int target;

    int nodes;
    dire **adjmat;
    std::vector<point_t *> the_way; //this is the way
    //special vector pointer for pathfinding

    victory roundstatus = inprog;
    int terrscore = 0;
    int counterscore = 0;

    int height;
    int width;
    CharMap *mapref;

    int roundtimer = ROUNDLEN;

    int Talive = 5;
    int Calive = 5;
    bool bombactive = false;

    int tspawnx;
    int tspawny;

    int cspawnx;
    int cspawny;

    bool player_terr;

    std::vector<ent_t *> projectile_v;
    std::queue<ent_t *> projectile_deletion;

    player_t *player;
    actor *terrent[5];
    actor *counent[5];

    Level(CharMap *bas);

    colision get_col(int x, int y);

    void add_ent(int x, int y, ent_t *entity);

    void rem_ent(int x, int y, ent_t *entity);

    void level_tick(char& ins);

    void print();

    void find_path(actor *myactor);

    void roundrestart() {
        Talive = 5;
        Calive = 5;
        roundstatus = inprog;
        roundtimer = ROUNDLEN;
        bombactive = false;
        target = rand() % 2 + 1;


        //remove bullets
        for (int i = projectile_v.size() - 1; i >= 0; i--) {
            rem_ent(projectile_v[i]->x_coord, projectile_v[i]->y_coord, projectile_v[i]);
            projectile_v[i]->die();
        }

        //clear instructions for ais and reset position
        for (int i = 0; i < 5; i++) {
            rem_ent(terrent[i]->x_coord, terrent[i]->y_coord, terrent[i]);

            while (!terrent[i]->ins_stack.empty()) {
                terrent[i]->ins_stack.pop();
            }


            rem_ent(counent[i]->x_coord, counent[i]->y_coord, counent[i]);

            while (!counent[i]->ins_stack.empty()) {
                counent[i]->ins_stack.pop();
            }

            terrent[i]->respawn();
            counent[i]->respawn();

        }

    }

    int get_index(int x, int y) {
        return point[x][y]->node_no;
    }


    void printMat() {//this function was just for testing;

        for (int i = 0; i < nodes; i++) {
            for (int j = 0; j < nodes; j++) {
                if (adjmat[i][j] == up) {
                    printw("u, ");
                } else if (adjmat[i][j] == down) {
                    printw("d, ");
                } else if (adjmat[i][j] == lef) {
                    printw("l, ");
                } else if (adjmat[i][j] == righ) {
                    printw("r, ");
                } else {
                    printw(" , ");
                }
            }

            printw("\n");

        }
    }

    ~Level() {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                delete point[i][j];
            }

            delete point[i];

        }

        delete[] point;

        for (int i = 0; i < nodes; i++) {
            delete adjmat[i];
        }

        delete[] adjmat;

    }
};

int main(int argc, char **argv) {
    //~ //	srand(time(NULL)); //Comment out to always have the same RNG for debugging
    CharMap *map = (argc == 2) ? new CharMap(argv[1]) : NULL; //Read in input file

    if (map == NULL) {
        printf("Invalid Map File\n");    //close if no file given
        return 1;
    }

    initCurses(); // Curses Initialisations

    srand(time(NULL));
    Level myLevel(map);
    char ch;
    printw("Welcome - Press Q to Exit\n");

    while ((ch) != 'q') {

        clear();
        myLevel.print();
        ch = getch();

        refresh();

        //myLevel.printMat();

        myLevel.level_tick(ch);


        //myLevel.print();

        printw("%c", ch);
        printw("\n");
    }



    //Your Code Here

    delete map;
    map = NULL;
    printw("\nPress any key to close\n");
    endCurses(); //END CURSES
    return 0;
}

void initCurses() {
    // Curses Initialisations
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
}
void endCurses() {
    refresh();
    getch(); //Make user press any key to close
    endwin();
}

void terroist_t::tick() {
    if (isalive) {
        if (ins_stack.empty()) {
            this->level_ptr->find_path(this);
        }

        if (!ins_stack.empty()) {
            if (ins_stack.top() == move_u) {
                this->move_up();
            } else if (ins_stack.top() == move_d) {
                this->move_down();
            } else if (ins_stack.top() == move_l) {
                this->move_left();
            } else if (ins_stack.top() == move_r) {
                this->move_right();
            }


            ins_stack.pop();
        }
    }
}


bomb_t::bomb_t(int x, int y, Level *lvlptr) {
    lvlptr->roundtimer = 31;
    level_ptr = lvlptr;
    this->x_coord = x;
    this->y_coord = y;
    this->velocity = 0;
    this->health = 30;
}



void bomb_t::tick() {
    this->health--;

    if (defusetime == 0) {
        //counter terrorists win
    }

    if (currdef) { //this is to make it so that the bomb has to be continously defused
        currdef = false;
    } else {
        defusetime = 5;
    }


    if (health <= 0) {
        die();
    }
}

//CharMap Functions
CharMap::CharMap(char *arg) {
    char temp;
    ifstream fin(arg);
    fin >> mapName;
    fin >> height;
    fin >> temp;
    fin >> width;
    map = new char *[height]; //Allocate our 2D array

    for (int i = 0; i < height; i++) {
        map[i] = new char[width];

        for (int j = 0; j < width; j++) { //Read into our array while we're at it!
            fin >> (map[i][j]) >> noskipws;    //dont skip whitespace
        }

        fin >> skipws; //skip it again just so we skip linefeed
    }

    //for(int i=0; i<height; i++){ //Uncomment if you have issues reading
    //    for(int j=0; j<width; j+g+) printf("%c", map[i][j]); printf("\n");};
}


CharMap::~CharMap() {
    if (map == NULL) {
        return;
    }

    for (int i = 0; i < height; i++) {
        delete [] map[i];
    }

    delete [] map;
}

void CharMap::print() { //call only after curses is initialized!
    printw("Read Map: '%s' with dimensions %dx%d!\n",
           mapName.c_str(), height, width);

    //Note the c_str is required for C++ Strings to print with printw
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            printw("%c", map[i][j]);
        }

        printw("\n");
    }
}


void actor::shoot() {
    ent_t *temp;
    temp = new bullet_t(this->x_coord, this->y_coord, this->direction,
                        this->is_terrorist, this->level_ptr);
    this->level_ptr->point[this->x_coord][this->y_coord]->entList.push_back(temp);
    this->level_ptr->projectile_v.push_back(temp);
}


void bullet_t::tick() {

    bool todie = false;

    if (notdead) {

        for (int i = 0; i < this->velocity; i++) {
            //check if next pos is wall
            if (direction == up) {
                if (this->level_ptr->get_col(this->x_coord - 1, this->y_coord) < Full) {

                    this->level_ptr->rem_ent(this->x_coord--,  this->y_coord, this);
                    this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
                    kill_enemies();
                } else {
                    todie = true;
                }
            }

            if (direction == down) {
                if (this->level_ptr->get_col(this->x_coord + 1, this->y_coord) < Full) {

                    this->level_ptr->rem_ent(this->x_coord++,  this->y_coord, this);
                    this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
                    kill_enemies();
                } else {
                    todie = true;
                }
            }

            if (direction == lef) {
                if (this->level_ptr->get_col(this->x_coord, this->y_coord - 1) < Full) {

                    this->level_ptr->rem_ent(this->x_coord,  this->y_coord--, this);
                    this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
                    kill_enemies();
                } else {
                    todie = true;
                }
            }

            if (direction == righ) {
                if (this->level_ptr->get_col(this->x_coord, this->y_coord + 1) < Full) {

                    this->level_ptr->rem_ent(this->x_coord,  this->y_coord++, this);
                    this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
                    kill_enemies();
                } else {
                    todie = true;
                }
            }
        }

        int x = this->x_coord;
        int y = this->y_coord;


        for (int i = 0; i < this->level_ptr->point[x][y]->entList.size(); i++) {
            if (this->level_ptr->point[x][y]->entList[i]->isactor
                && this->level_ptr->point[x][y]->entList[i]->is_terrorist !=
                this->is_terrorist) {
                this->level_ptr->point[x][y]->entList[i]->die();
            }
        }

        if (todie) {
            die();
        }
    }
}

void bullet_t::die() {
    //remove from curent location
    notdead = false;
    this->level_ptr->rem_ent(this->x_coord,  this->y_coord, this);
    //add to projectile_deletion
    this->level_ptr->projectile_deletion.push(this);

}

bullet_t::~bullet_t() {

}


bool actor::move_up() {
    if ((this->level_ptr->get_col(this->x_coord - 1, this->y_coord) < Thin)
        && ((this->is_in_tunnel == Non) || (this->is_in_tunnel == Vertical))) {
        if (this->isalive) {
            this->level_ptr->rem_ent(this->x_coord--,  this->y_coord, this);
            this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
            this->direction = up;

            if (this->level_ptr->get_col(this->x_coord, this->y_coord) == Tunnel) {
                this->is_in_tunnel = Vertical;
            } else {
                this->is_in_tunnel = Non;
            }
        }

        return true;
    } else {
        return false;
    }
}

bool actor::move_down() {
    if ((this->level_ptr->get_col(this->x_coord + 1, this->y_coord) < Thin)
        && ((this->is_in_tunnel == Non) || (this->is_in_tunnel == Vertical))) {
        if (this->isalive) {
            this->level_ptr->rem_ent(this->x_coord++,  this->y_coord, this);
            this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
            this->direction = down;

            if (this->level_ptr->get_col(this->x_coord, this->y_coord) == Tunnel) {
                this->is_in_tunnel = Vertical;
            } else {
                this->is_in_tunnel = Non;
            }
        }

        return true;
    } else {
        return false;
    }
}

bool actor::move_left() {
    if ((this->level_ptr->get_col(this->x_coord, this->y_coord - 1) < Thin)
        && ((this->is_in_tunnel == Non) || (this->is_in_tunnel == Horizontal))) {
        if (this->isalive) {
            this->level_ptr->rem_ent(this->x_coord,  this->y_coord--, this);
            this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
            this->direction = lef;

            if (this->level_ptr->get_col(this->x_coord, this->y_coord) == Tunnel) {
                this->is_in_tunnel = Horizontal;
            } else {
                this->is_in_tunnel = Non;
            }
        }

        return true;
    } else {
        return false;
    }
}

void actor::respawn() {
    this->x_coord = this->xspawn;
    this->y_coord = this->yspawn;
    this->isalive = true;
    this->level_ptr->add_ent(this->x_coord, this->y_coord, this);

}

void actor::die() {
    this->isalive = false;

    this->level_ptr->rem_ent(this->x_coord, this->y_coord, this);

    if (this->is_terrorist) {
        this->level_ptr->Talive--;
    } else {
        this->level_ptr->Calive--;
    }
}

bool actor::move_right() {
    if ((this->level_ptr->get_col(this->x_coord, this->y_coord + 1) < Thin)
        && ((this->is_in_tunnel == Non) || (this->is_in_tunnel == Horizontal))) {
        if (this->isalive) {
            this->level_ptr->rem_ent(this->x_coord,  this->y_coord++, this);
            this->level_ptr->add_ent(this->x_coord,  this->y_coord, this);
            this->direction = righ;

            if (this->level_ptr->get_col(this->x_coord, this->y_coord) == Tunnel) {
                this->is_in_tunnel = Horizontal;
            } else {
                this->is_in_tunnel = Non;
            }
        }

        return true;
    } else {
        return false;
    }
}

Level::Level(CharMap *bas) {
    target = rand() % 2 + 1;
    mapref = bas;
    height = mapref->height;
    width = mapref->width;
    point = new point_t **[height];

    roundtimer = ROUNDLEN;


    printw("Would you like to play as a Terrorist(t) or a Counter(c)\n");
    char inp = ' ';

    while ((inp != 'c') && (inp != 't')) {
        inp = getch();
    }

    if (inp == 't') {
        player_terr = true;
    } else {

        player_terr = false;
    }

    clear();

    nodes = 0;


    for (int i = 0; i < height; i++) {
        point[i] = new point_t *[width];
    }

    char temp;

    //generate the map

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            temp = mapref->map[i][j];

            if (temp == ' ') {
                point[i][j] = new empty_t(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
            } else if (temp == 'n') {
                point[i][j] = new wall_t(i, j);

            } else if (temp == 'x') {
                point[i][j] = new wall_t(i, j);
            } else if (temp == 'o') {
                point[i][j] = new obsticle_t(i, j);
            } else if (temp == 'P') {
                point[i][j] = new site_t(i, j, nodes, this);
                the_way.push_back(point[i][j]);
                nodes++;
            } else if (temp == '1') {
                point[i][j] = new site_1(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
            } else if (temp == '2') {
                point[i][j] = new site_2(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
            } else if (temp == '3') {
                point[i][j] = new site_3(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
            } //else if (temp == '@') {//this one is just for testing
            //~ point[i][j] = new empty_t(i, j);
            //~ player = new player_t(i, j, player_terr, this);
            //~ point[i][j]->entList.push_back(player);
            else if (temp == '#') {
                point[i][j] = new tunnel_t(i, j, nodes);
                the_way.push_back(point[i][j]);
                the_way.push_back(point[i][j]);
                nodes++;
                nodes++;
            } else if (temp == 'T') {
                point[i][j] = new empty_t(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
                tspawnx = i;
                tspawny = j;
            } else if (temp == 'C') {
                point[i][j] = new empty_t(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
                cspawnx = i;
                cspawny = j;
            } else {
                point[i][j] = new empty_t(i, j, nodes);
                the_way.push_back(point[i][j]);
                nodes++;
            }


        }
    }

    //spawn in the players

    for (int i = 0; i < 4; i++) {
        //location is slightly randomized
        int rng = rand() % 3 - 1;

        //spawn terrorists
        terrent[i] = new terroist_t(tspawnx + rng, tspawny - i, this);
        point[tspawnx + rng][tspawny - i]->entList.push_back(terrent[i]);

        rng = rand() % 3 - 1;

        //spawn counters
        counent[i] = new counter_t(cspawnx + rng, cspawny - i, this);
        point[cspawnx + rng][cspawny - i]->entList.push_back(counent[i]);
    }

    //spawn in the player and the last member of the opposing team
    if (player_terr) {
        player = new player_t(tspawnx, tspawny - 4, player_terr, this);
        terrent[4] = player;
        point[tspawnx][tspawny - 4]->entList.push_back(terrent[4]);
        counent[4] = new counter_t(cspawnx, cspawny - 4, this);

        point[cspawnx][cspawny - 4]->entList.push_back(counent[4]);
    } else {
        player = new player_t(cspawnx, cspawny - 4, player_terr, this);
        counent[4] = player;

        point[cspawnx][cspawny - 4]->entList.push_back(counent[4]);
        terrent[4] = new terroist_t(tspawnx, tspawny - 4, this);

        point[tspawnx][tspawny - 4]->entList.push_back(terrent[4]);
    }

    adjmat = new dire* [nodes];

    for (int i = 0; i < nodes; i++) {
        adjmat[i] = new dire[nodes];
    }

    for (int i = 0; i < nodes; i++) {
        for (int j = 0; j < nodes; j++) {
            adjmat[i][j] = noned;
        }
    }

    //go through the map one more time
    //generate adj matrix
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (get_col(i, j) < Tunnel) {
                adjmat[point[i][j]->node_no][point[i][j]->node_no] = self;

                if (get_col(i - 1, j) < Thin) {
                    adjmat[point[i][j]->node_no][point[i - 1][j]->node_no] = up;
                }

                if (get_col(i + 1, j) < Thin) {
                    adjmat[point[i][j]->node_no][point[i + 1][j]->node_no] = down;
                }

                if (get_col(i, j - 1) < Tunnel) {
                    adjmat[point[i][j]->node_no][point[i][j - 1]->node_no] = lef;
                }

                if (get_col(i, j - 1) == Tunnel) {
                    adjmat[point[i][j]->node_no][point[i][j - 1]->node_no + 1] = lef;
                }

                if (get_col(i, j + 1) < Tunnel) {
                    adjmat[point[i][j]->node_no][point[i][j + 1]->node_no] = righ;
                }

                if (get_col(i, j - 1) == Tunnel) {
                    adjmat[point[i][j]->node_no][point[i][j + 1]->node_no + 1] = righ;
                }
            } else if (get_col(i, j) == Tunnel) {
                adjmat[point[i][j]->node_no][point[i][j]->node_no] = self;
                adjmat[point[i][j]->node_no + 1][point[i][j]->node_no + 1] = self;

                if (get_col(i - 1, j) < Thin) {
                    adjmat[point[i][j]->node_no][point[i - 1][j]->node_no] = up;
                }

                if (get_col(i + 1, j) < Thin) {
                    adjmat[point[i][j]->node_no][point[i + 1][j]->node_no] = down;
                }

                if (get_col(i, j - 1) < Tunnel) {
                    adjmat[point[i][j]->node_no + 1][point[i][j - 1]->node_no] = lef;
                }

                if (get_col(i, j - 1) == Tunnel) {
                    adjmat[point[i][j]->node_no + 1][point[i][j - 1]->node_no + 1] = lef;
                }

                if (get_col(i, j + 1) < Tunnel) {
                    adjmat[point[i][j]->node_no + 1][point[i][j + 1]->node_no] = righ;
                }

                if (get_col(i, j - 1) == Tunnel) {
                    adjmat[point[i][j]->node_no + 1][point[i][j + 1]->node_no + 1] = righ;
                }
            }
        }
    }
}

int minDist(int dist[], bool vis[], int size) {

    int min = 9999;
    int index;

    for (int i = 0; i < size; i++) {

        if (vis[i] == false && dist[i] <= min) {
            min == dist[i];
            index = i;
        }
    }

    return index;

}


void Level::find_path(actor *myactor) {


    //uses dijkstras
    int distance[nodes];
    bool visited[nodes];
    int parent[nodes];


    int act_ind = get_index(myactor->x_coord, myactor->y_coord);
    parent[act_ind] = -1;


    for (int i = 0; i < nodes; i++) {
        distance[i] = INT_MAX;
        visited[i] = false;
    }

    distance[act_ind] = 0;


    for (int count = 0; count < nodes - 1; count++) {

        //get the currently smallest unvisited path
        int u = minDist(distance, visited, nodes);

        visited[u] = true;

        for (int j = 0; j < nodes; j++) {
            if (!visited[j] && adjmat[u][j] != noned && distance[j] > (distance[u] + 1)) {
                distance[j] = distance[u] + 1;
                parent[j] = u;
            }
        }


    }

    //paths are found
    //get path with app conditions
    //give instructions to ai

    int next;
    int currmin = INT_MAX;

    for (int i = 0; i < nodes; i++) {
        if (the_way[i]->isbombsite && distance[i] < currmin
            && target == the_way[i]->siteno) {
            next = i;
            currmin = distance[i];
        }
    }

    int root = parent[next];

    while (root != -1) {
        if (adjmat[root][next] == up) {
            myactor->ins_stack.push(move_u);
        } else if (adjmat[root][next] == down) {
            myactor->ins_stack.push(move_d);
        } else if (adjmat[root][next] == lef) {
            myactor->ins_stack.push(move_l);
        } else if (adjmat[root][next] == righ) {
            myactor->ins_stack.push(move_r);
        }

        next = root;
        root = parent[root];
    }

}

colision Level::get_col(int x, int y) {

    return point[x][y]->has_colision;

}


void Level::add_ent(int x, int y, ent_t *entity) {
    point[x][y]->entList.push_back(entity);//getting segfault here
    //I dont see why I am just pushing a pointer onto a vector
    return;
}



void Level::level_tick(char& ins) {
    roundtimer--;

    if (roundtimer < 0) {
        if (!bombactive) {
            roundstatus = Counter_vic;
        } else {
            roundstatus = Terroist_vic;
        }
    }

    if (Talive == 0 && !bombactive) {
        roundstatus = Counter_vic;
    }

    if (Calive == 0) {
        roundstatus = Terroist_vic;
    }

    if (roundstatus == Terroist_vic) {
        terrscore++;
        roundrestart();
    }

    else if (roundstatus == Counter_vic) {
        counterscore++;
        roundrestart();
    }

    else {
        while (!(player->ptick(ins))) {
            ins = getch();
        }

        for (int i = 0; i < 5; i++) {
            terrent[i]->tick();
            counent[i]->tick();
        }

        //~ //do projectiles here

        for (int i = 0; i < projectile_v.size(); i++) {
            projectile_v[i]->tick();
        }

        ent_t *temp;

        while (projectile_deletion.size() != 0) {
            temp = projectile_deletion.front();

            int marked = -1;

            for (int i = 0; i < projectile_v.size(); i++) {
                if (projectile_v[i] == temp) {

                    marked = i;
                }
            }

            if (marked != -1) {

                projectile_v.erase(projectile_v.begin() +
                                   marked);
                delete temp;
                printw("deleted temp");
                projectile_deletion.pop();

            }

        }
    }
}

void Level::rem_ent(int x, int y, ent_t *entity) {
    int marked = -1;

    for (int i = 0; i < point[x][y]->entList.size(); i++) {
        if (point[x][y]->entList[i] == entity) {
            marked = i;
        }
    }

    if (marked == -1) {
        return;
    } else {
        point[x][y]->entList.erase(point[x][y]->entList.begin() + marked);
        return;
    }

}

void Level::print() {

    printw("Terrorist score: ");
    printw("%i", terrscore);
    printw("   Counter score: ");
    printw("%i", counterscore);
    printw("    Round time: ");
    printw("%i", roundtimer);
    printw("\n\n");

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            point[i][j]->printP();
        }

        printw("\n");
    }
}

void counter_t::tick() {
    if (isalive) {
        if (ins_stack.empty()) {
            this->level_ptr->find_path(this);
        }

        if (!ins_stack.empty()) {
            if (ins_stack.top() == move_u) {
                this->move_up();
            } else if (ins_stack.top() == move_d) {
                this->move_down();
            } else if (ins_stack.top() == move_l) {
                this->move_left();
            } else if (ins_stack.top() == move_r) {
                this->move_right();
            }


            ins_stack.pop();
        }
    }
}

site_t::site_t(int x, int y, int node, Level *lvlptr) {
    this->node_no = node;

    if (lvlptr->point[x - 1][y]->isbombsite) {
        this->siteno = lvlptr->point[x - 1][y]->siteno;
    }

    if (lvlptr->point[x][y - 1]->isbombsite) {
        this->siteno = lvlptr->point[x][y - 1]->siteno;
    }

    //look to left to see if thats a bomb site
    //look up to see if that is a bomob site
    //if it is a bomb site set your siteno to theirs
    this->isbombsite = true;
    this->symbol = 'P';
    this->has_colision = None;
    this->blocks_view = false;
    this->x_coord = x;
    this->y_coord = y;
}

bomb_i::bomb_i(int x, int y, Level *lvlptr) {
    this->level_ptr = lvlptr;

    this->x_coord = x;
    this->y_coord = y;
    this->symbol = 'B';
}

void bomb_i::tick() {
    int x = this->x_coord;
    int y = this->y_coord;


    for (int i = 0; i < this->level_ptr->point[x][y]->entList.size(); i++) {
        if (this->level_ptr->point[x][y]->entList[i]->isactor
            && this->level_ptr->point[x][y]->entList[i]->is_terrorist) {
            this->level_ptr->point[x][y]->entList[i]->has_bomb = true;
            this->level_ptr->rem_ent(x, y, this);
        }
    }

}
