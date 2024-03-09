#include <iostream>
#include <unistd.h> // For the sleep function
#include <math.h>

// Settings
const int screenWidth = 80;
const int screenHeight = 20;
std::string textColors[8] = {"\033[30m","\033[31m","\033[32m","\033[33m","\033[34m","\033[35m","\033[36m","\033[37m"};
std::string backColors[8] = {"\033[40m","\033[41m","\033[42m","\033[43m","\033[44m","\033[45m","\033[46m","\033[47m"};

// Debug settings
const bool debugDir {true}; // Shows direction things are facing


struct ivec2 {
    int x {0};
    int y {0};
    
    ivec2(): x(0), y(0) {}
    ivec2(int x, int y): x(x), y(y) {}
};


// I think this takes in lefthandside (lhs) and righthandside(rhs)
bool operator==(const ivec2& lhs, const ivec2& rhs) {
    if(lhs.x == rhs.x && lhs.y == rhs.y) {return true;}
    return false;
}

void operator+=(ivec2& lhs, const ivec2& rhs) {
    lhs.x += rhs.x; lhs.y += rhs.y;
}

ivec2 operator-(ivec2 lhs, const ivec2 rhs) {
    return ivec2(lhs.x-rhs.x,lhs.y-rhs.y);
}

float length(ivec2 vector) {return sqrt(vector.x*vector.x + vector.y*vector.y);}


struct colored_char {
    char character;
    int textColor; bool hasText;
    int backColor; bool hasBack;
    
    colored_char(): character('?'), textColor(1), backColor(7), hasText(true), hasBack(false) {}
    colored_char(char character, int textColor, int backColor, bool hasText, bool hasBack):
        character(character), textColor(textColor), backColor(backColor), hasText(hasText), hasBack(hasBack) {}
};


class Buffer {
    char text[screenWidth][screenHeight] {'1'};
    int textColor[screenWidth][screenHeight] {7};
    int backColor[screenWidth][screenHeight] {0};
    
    public:
    
    void clear() {
        for(int y = 0; y < screenHeight; y++) {
            for(int x = 0; x < screenWidth; x++) {
                textColor[x][y] = 7;
                backColor[x][y] = 0;
                text[x][y] = '-';
            }
        }
    }
    
    // Read and write functions
    void write(char textin, ivec2 pixel) {text[pixel.x][pixel.y] = textin;}
    void color_text(int color, ivec2 pixel) {textColor[pixel.x][pixel.y] = color;}
    void color_back(int color, ivec2 pixel) {backColor[pixel.x][pixel.y] = color;}
    char read(ivec2 pixel) {return text[pixel.x][pixel.y];}
    int get_textcolor(ivec2 pixel) {return textColor[pixel.x][pixel.y];}
    int get_backcolor(ivec2 pixel) {return backColor[pixel.x][pixel.y];}
    
    void draw() {
        std::cout<<"\033[0;0H";
        for(int y = 0; y < screenHeight; y++) {
            for(int x = 0; x < screenWidth; x++) {
                std::cout<<textColors[textColor[x][y]];
                std::cout<<backColors[backColor[x][y]];
                std::cout<<text[x][y];
            }
            std::cout<<"\n";
        }
    }
};

// Crop growth stages
// 0: Wheat;  0-5 growing; 6 ready; 7+ withered;
// 1: Barley; 0-5 growing; 6 ready; 7+ withered;
// 2: Oats;   0-4 growing; 5 ready; 6+ withered;
// 3: Canola; 0-6 growing; 7 ready; 8+ withered;
// 4: Cotton; 0-4 growing; 5 ready; 6+ withered;

int growthStages[5][2] = {
    {6,7},
    {6,7},
    {5,6},
    {7,8},
    {5,6}
};

// 0-3 crop prices now calculated by (1000/yield)*(6/growthTime) --
// -- and transport price is 0.04 per unit
// Crop prices - base   - transport included
// 0: Wheat;     1.40                   1.44
// 1: Barley;    1.32                   1.36
// 2: Oats;      0.92                   0.96
// 3: Canola;    2.68                   2.72
// 4: Cotton;    2.35                   2.39

float cropPrices[5] = {
    1.44,
    1.36,
    0.96,
    2.72,
    2.39
};

// Yield per acre
float cropYields[5] = {
    714.,
    760.,
    909.,
    435.,
    355
};


class Bale {
    ivec2 pos {2,0};
    int cropID {0}; float fillLevel {0};
    bool active {false};
    
    public:
    
    Bale(int cropID, float fillLevel): cropID(cropID), fillLevel(fillLevel) {}
    Bale() {}
    
    bool is_active() {return active;}
    int get_cropID() {return cropID;}
    ivec2 get_pos() {return pos;}
    
    void spawn(ivec2 p) {pos = p; active = true;}
    void despawn() {active = false;}
    
    void draw(Buffer* b) {b->color_text(7,pos); b->write('o',pos);}
};

Bale bales[32]; // Not wanting to need too many

void spawn_bale(ivec2 pos) {
    for(int i = 0; i < 32; i++) {
        if(!bales[i].is_active()) {bales[i].spawn(pos); break;}
}   }


struct harvestData {
    int ID {-1};
    float yield {0.};
    
    harvestData(int ID, float yield): ID(ID), yield(yield) {}
};

class Tile {
    colored_char character;
    // Crop data
    bool isCrop {false}; int growthStage {0}; int cropID {0};
    bool readyToHarvest {true}; bool withered {false};
    //
    
    public:
    
    Tile(): character() {}
    Tile(char letter, int tc, int bc, bool hastc, bool hasbc): character(letter,tc,bc,hastc,hasbc) {}
    Tile(colored_char letter): character(letter) {}
    
    bool is_crop() {return isCrop;}
    bool is_ready() {return readyToHarvest;}
    bool is_withered() {return withered;}
    int get_cropID() {return cropID;}
    
    void set_is_crop(bool state) {isCrop = state;}
    void set_cropID(int ID) {cropID = ID;}
    
    void process() {
        // Crop processing
        if(isCrop) {
            growthStage++;
            if(growthStage == growthStages[cropID][0]) {
                readyToHarvest = true;
            }
            else if(growthStage >= growthStages[cropID][1]) {
                withered = true;
            }
            else {
                readyToHarvest = false;
                withered = false;
            }
        }
    }
    
    colored_char get_char() {return character;}
    void set_char(colored_char c) {character = c;}
    
    bool is_similar_to(Tile tile) {
        // This function is a weird one! It checks to see if it and another tile
        // are similar enough.
        if(isCrop == tile.is_crop() && cropID == tile.get_cropID() && readyToHarvest == tile.is_ready()) {return true;}
        else {return false;} // Should definitely add more in the future <3
    }
};


colored_char get_char (int ID, ivec2 pos) {
    switch (ID) {
        case 0:
            // Grass
            if((pos.x+pos.y)%2 == 0) {
                return colored_char(',',2,0,true,true);
            }
            return colored_char('`',2,0,true,true);
        case 1:
            // Soil
            return colored_char('-',0,1,true,true);
        case 2:
            // Seed
            return colored_char(',',2,1,true,true);
        case 3:
            // Wheat
            return colored_char('w',2,1,true,true);
        case 4:
            // Barley
            return colored_char('b',2,1,true,true);
        case 5:
            // Oats
            return colored_char('o',2,1,true,true);
        case 6:
            // Canola
            return colored_char('c',2,1,true,true);
        case 7:
            // Cotton
            return colored_char('c',7,1,true,true);
        case 8:
            // Concrete
            return colored_char(' ',7,7,true,true);
    }
    return colored_char();
}

class World {
    int tileIDs[screenWidth][screenHeight] {0};
    Tile tiles[screenWidth][screenHeight];
    
    public:
    
    void generate_rectangle(ivec2 topLeft, ivec2 bottomRight, int ID) {
        for(int x = 0; x <= bottomRight.x; x++) {
            for(int y = bottomRight.y; y <= topLeft.y; y++) {
                tiles[x][y] = Tile(get_char(ID,ivec2(x,y)));
                tileIDs[x][y] = ID;
            }
        }
    }
    
    void generate() {
        for(int y = 0; y < screenHeight; y++) {
            for(int x = 0; x < screenWidth; x++) {
                float dx = 20. - float(x)/2.;
                float dy = 10. - float(y);
                float d = sqrt(dx*dx + dy*dy);
                if(d < 10.) {
                    tileIDs[x][y] = 7;
                    //tiles[x][y] = Tile(get_char(7,ivec2(x,y)));
                    tiles[x][y].set_is_crop(true);
                    tiles[x][y].set_cropID(4);
                }
                else {
                    tileIDs[x][y] = 0;
                    tiles[x][y] = Tile(get_char(0,ivec2(x,y)));
                }
            }
        }
    }
    
    void update() {
        for(int x = 0; x < screenWidth; x++) {
            for(int y = 0; y < screenHeight; y++) {
                tiles[x][y].set_char(get_char(tileIDs[x][y],ivec2(x,y)));
            }
        }
    }
    
    void load_to(Buffer* b) {
        for(int y = 0; y < screenHeight; y++) {
            for(int x = 0; x < screenWidth; x++) {
                colored_char c = tiles[x][y].get_char();
                if(c.hasText) {
                    b->write(c.character,ivec2(x,y));
                    b->color_text(c.textColor,ivec2(x,y));
                }
                if(c.hasBack) {
                    b->color_back(c.backColor,ivec2(x,y));
                }
            }
        }
    }
    
    int get_cropID(ivec2 pos) {
        if(tiles[pos.x][pos.y].is_crop()) {
            return tiles[pos.x][pos.y].get_cropID();
        }
        return -1; // No crop here!
    }
    
    Tile* get_tile(ivec2 pos) {return &tiles[pos.x][pos.y];}
    
    ivec2 find_closest_tile(ivec2 pos, Tile tile) {
        float closestDist = float(screenWidth+screenHeight);
        ivec2 closestPos = pos;
        
        for(int x = 0; x < screenWidth; x++) {
            for(int y = 0; y < screenHeight; y++) {
                if(tile.is_similar_to(tiles[x][y])) {
                    float dx = float(pos.x-x);
                    float dy = float(pos.y-y);
                    float d = sqrt(dx*dx + dy*dy);
                    if(d < closestDist) {
                        closestDist = d;
                        closestPos = ivec2(x,y);
                    }
                }
            }
        }
        return closestPos;
    }
    
    harvestData harvest(ivec2 pos) {
        harvestData result(-1.,0.);
        Tile* tile = &tiles[pos.x][pos.y]; // Doesn't need to be a pointer (pretty confident)
        if(tile->is_crop() && tile->is_ready()) {
            // This needs to set tile to 'uncultivated' state, but we don't have
            // that yet. Patience please <3
            result.ID = tile->get_cropID();
            result.yield = cropYields[tile->get_cropID()];
            tileIDs[pos.x][pos.y] = 1;
        }
        return result;
    }
};


void draw_debug_dir(ivec2 pos, ivec2 dir, Buffer* b) {
    if(debugDir && (dir.x != 0 || dir.y != 0)) {
        ivec2 pos2(pos.x+dir.x,pos.y+dir.y);
        b->color_text(7,pos2);
        b->color_back(0,pos2);
        switch(dir.x) {
            case -1:
                switch(dir.y) {
                    case -1:
                        b->write('\\',pos2);
                        break;
                    case 0:
                        b->write('-',pos2);
                        break;
                    case 1:
                        b->write('/',pos2);
                        break;
                }
                break;
            case 0:
                b->write('|',pos2);
                break;
            case 1:
                switch(dir.y) {
                    case -1:
                        b->write('/',pos2);
                        break;
                    case 0:
                        b->write('-',pos2);
                        break;
                    case 1:
                        b->write('\\',pos2);
                        break;
                }
                break;
        }
    }
}


class Worker {
    ivec2 pos {3,4}; float movementOpportunity {0.}; float walkingSpeed {1.};
    ivec2 dir {0,0};
    bool active {false};
    bool usingEquipment {false};
    bool usingVehicle {false}; int vehicleID {0};
    char name {'#'};
    int currentTaskID {0}; int taskStage {0};
    ivec2 target {25,10}; bool hasTarget {true};
    bool taskComplete {false};
    
    public:
    
    Worker(char name): name(name) {}
    
    void set_task(int taskID) {active = true; currentTaskID = taskID;}
    
    void set_target(ivec2 newTarget) {target = newTarget; hasTarget = true;}
    void set_walking_speed(float speed) {walkingSpeed = speed;}
    void set_using_vehicle(bool state) {usingVehicle = state;}
    void set_using_equipment(bool state) {usingEquipment = state;}
    ivec2 get_pos() {return pos;}
    char get_name() {return name;}
    void get_inside() {}
    bool is_task_complete() {return taskComplete;}
    bool is_active() {return active;}
    bool is_using_vehicle() {return usingVehicle;}
    bool is_using_equipment() {return usingEquipment;}
    
    void draw(Buffer* b) {
        b->color_text(6,pos);
        b->write(name,pos);
        draw_debug_dir(pos,dir,b);
    }
    
    void task_goto() {
        dir = ivec2(0,0);
        
        if(target.x < pos.x) {dir.x = -1;}
        else if(target.x > pos.x) {dir.x = 1;}
        
        if(target.y < pos.y) {dir.y = -1;}
        else if(target.y > pos.y) {dir.y = 1;}
        
        movementOpportunity += walkingSpeed;
        if(movementOpportunity >= length(dir)) {
            pos += dir; movementOpportunity = 0.;
        }
        
        if(target.x == pos.x && target.y == pos.y) {
            taskComplete = true; hasTarget = false;
        }
    }
    
    void task_goto(ivec2 targetPos) {target = targetPos; task_goto();}
    
    void process(World* world) {
        if(!active) {
            return;
        }
        
        switch(currentTaskID) {
            // IDLE
            case 0:
                break;
            
            // GOTO pos
            case 1:
                task_goto();
                break;
        }
        
        if(taskComplete) {
            currentTaskID = 0;
        }
    }
};

Worker workers[7] {'a','b','c','d','e','f','g'};


class Harvester {
    ivec2 pos;
    ivec2 dir {0,0};
    bool harvestIDs[5] {false,false,false,false,true};
    bool hasTarget {false}; ivec2 target;
    ivec2 targetDir {0,0};
    int targetedCropID {0};
    bool hasWorker {false};
    
    int containsCrop {false}; float fillLevel {0.}; float capacity {3500.};
    
    public:
    
    void set_pos(ivec2 newpos) {pos = newpos;}
    ivec2 get_pos() {return pos;}
    
    void allow_harvest(int cropID) {harvestIDs[cropID] = true;}
    
    void draw(Buffer* b) {
        b->color_text(6,pos);
        b->write('[',pos);
        draw_debug_dir(pos,dir,b);
    }
    
    bool go_harvest(ivec2 cropPos, World* w) {
        int cropID = w->get_cropID(cropPos);
        if(cropID == -1) {return false;} //No harvestable crop at cropPos
        if(harvestIDs[cropID]) {hasTarget = true; target = cropPos; return true;}
        return false;
    }
    
    bool get_inside(Worker* w) {
        if(hasWorker) {return false;} // Worker already inside
        int dx = w->get_pos().x - pos.x;
        int dy = w->get_pos().y - pos.y;
        if(abs(dx) <= 1 && abs(dy) <= 1) {
            hasWorker = true;
            // Should probably replace or remove workerID with a pointer, but I
            // think the task manager can handle that, even if it's a little
            // weird
            // Note: The task manager handled it :D
            return true; // Worker able to get inside
        }
        return false; // Worker too far from vehicle
    }
    
    bool get_inside(int ID) {return get_inside(&workers[ID]);}
    
    void process(World* world) {
        Tile* tile = world->get_tile(pos);
        // Harvest if on tile
        // Not added: Needs to check if targetedCropID is the same as what is
        // on the tile so it doesn't try to mix two different crop types and
        // confuse the system
        int cropID = tile->get_cropID();
        if(cropID >= 0) {
            if(harvestIDs[cropID]) {
                fillLevel += world->harvest(pos).yield;
            }
        }
        if(fillLevel >= capacity) {
            fillLevel -= capacity;
            spawn_bale(pos-dir); // Unload bale
        }
    }
    
} cottonCombine, grainCombine;

Harvester harvesters[2] = {cottonCombine, grainCombine};


class Task {
    Worker* w;
    Harvester* h;
    ivec2 target;
    int programCounter {0};
    int taskMemory[64] {0};
    int taskMemoryLength;
    bool taskStarted {false};
    
    public:
    
    Task(Worker* w): w(w) {}
    
    void set_target(ivec2 t) {target = t;}
    void set_worker(Worker* worker) {w = worker;}
    void set_harvester(Harvester* harvester) {h = harvester;}
    
    void goto_target() {w->task_goto(target);}
    void goto_harvester() {w->task_goto(h->get_pos());}
    
    void appendTaskMemory(int value) {taskMemory[taskMemoryLength] = value; taskMemoryLength++;}
    
    void process() {
        switch(taskMemory[programCounter]) {
            // Idle
            case 0:
                break;
            
            // Goto
            case 1:
                if(!taskStarted) {
                    target.x = taskMemory[programCounter+1];
                    target.y = taskMemory[programCounter+2];
                    w->set_target(target);
                    taskStarted = true;
                }
                if(target == w->get_pos()) {
                    taskStarted = false;
                    programCounter += 3;
                    break;
                }
                w->task_goto();
                if(w->is_using_vehicle()) {h->set_pos(w->get_pos());}
                break;
            
            // Jump
            case 2:
                taskStarted = false;
                programCounter = taskMemory[programCounter+1];
                break;
            
            // Set harvester
            case 3:
                h = &harvesters[taskMemory[programCounter+1]];
                programCounter += 2;
                break;
            
            // Set target to harvester
            case 4:
                target = h->get_pos();
                w->set_target(target);
                programCounter++;
                break;
            
            // Goto target
            case 5:
                if(!taskStarted) {
                    w->set_target(target);
                    taskStarted = true;
                }
                if(target == w->get_pos()) {
                    taskStarted = false;
                    programCounter++;
                    break;
                }
                w->task_goto();
                break;
            
            // Get inside harvester
            case 6:
                if(w->is_using_vehicle() || w->is_using_equipment()) {break;}
                if(h->get_inside(w)) {
                    w->set_using_vehicle(true);
                    programCounter++;
                }
        }
    }
};

int gameTime = 0;

int main()
{
    Buffer buffer; World world; world.generate();
    world.generate_rectangle(ivec2(2,10),ivec2(10,2),8);
    //workers[0].set_task(1);
    //workers[1].set_task(1); workers[1].set_target(ivec2(1,1));
    
    // Worker A
    Task tasksetA(&workers[0]);
    tasksetA.appendTaskMemory(1); // Goto
    tasksetA.appendTaskMemory(25);
    tasksetA.appendTaskMemory(10);
    tasksetA.appendTaskMemory(1); // Goto
    tasksetA.appendTaskMemory(5);
    tasksetA.appendTaskMemory(15);
    tasksetA.appendTaskMemory(2); // Jump
    tasksetA.appendTaskMemory(0);
    
    // Worker B
    Task tasksetB(&workers[1]);
    tasksetB.appendTaskMemory(3); // Set harvester ID to 0
    tasksetB.appendTaskMemory(0);
    tasksetB.appendTaskMemory(4); // Target harvester
    tasksetB.appendTaskMemory(5); // Goto target
    tasksetB.appendTaskMemory(6); // Get inside harvester
    tasksetB.appendTaskMemory(1); // Goto
    tasksetB.appendTaskMemory(35);
    tasksetB.appendTaskMemory(12);
    
    std::cout<<"\033[?25l";
    while(true) {
        buffer.clear(); world.load_to(&buffer);
        // Process tasks
        tasksetA.process();
        tasksetB.process();
        // Process workers
        for(int i = 0; i < 7; i++) {
            workers[i].process(&world);
        }
        // Process vehicles
        for(int i = 0; i < 2; i++) {
            harvesters[i].process(&world);
        }
        
        world.update();
        world.load_to(&buffer);
        
        // Draw workers
        for(int i = 0; i < 7; i++) {
            workers[i].draw(&buffer);
        }
        // Draw bales
        for(int i = 0; i < 32; i++) {
            if(bales[i].is_active()) {bales[i].draw(&buffer);}
        }
        // Draw vehicles
        for(int i = 0; i < 2; i++) {
            harvesters[i].draw(&buffer);
        }
        buffer.draw();
        
        sleep(0.2); gameTime++;
    }
    
    return 0;
}



















