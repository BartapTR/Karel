#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>

using std::vector;
using std::array;
using std::cout;
using std::string;
using std::endl;

// A position always has exactly three coordinates. std::array stores its
// elements directly (unlike vector, it cannot grow or shrink), so it suits
// fixed-size data such as one XYZ position.
using Position = array<float, 3>;
vector<vector<string>> functions;
vector<string> functionen;

// getClosestBallInfo returns one compact snapshot with this fixed layout:
// [distance, delta X, delta Y, delta Z, index in balls].
// Keeping related values together means callers can find the nearest ball
// once and reuse all of its data instead of rescanning the vector per value.
using ClosestBallInfo = array<float, 5>;

// Named offsets make the array's layout readable at each use site and avoid
// scattering unexplained numeric subscripts such as closestBall[3].
constexpr size_t DISTANCE_INDEX = 0;
constexpr size_t DELTA_X_INDEX = 1;
constexpr size_t DELTA_Y_INDEX = 2;
constexpr size_t DELTA_Z_INDEX = 3;
constexpr size_t BALL_INDEX = 4;

string lastCommand;

Position position = {0, 0, 0};
vector<Position> balls = {};
//the shape of area is a sphere

//After that you can add numbers and command codes
//Maybe gravity, time and saving
//Turn it into an actual game?

float radius = 30.0;

int xydirection = 0;
int xzdirection = 0;
float normalMovement = 1.0;
const double PI = 3.1416;

float RMS(vector<float> numbas){
    float rms;
    for (float i:numbas){
        rms += i*i;
    }
    rms = std::sqrt(rms);
    return rms;
}

int powar(int numba, int powa){
    int out = 1;
    while(true){
        if (powa%2 == 0 && powa){
            numba *= numba;
            powa /= 2;
        }else if (powa >1){
            powa-=1;
            out *= numba;
        }else{
            break;
        }
    }
    out *= numba;
    return out;
}

int toBinary(string inphut){
    size_t lent = inphut.length();
    int numbir = 0;
    for(size_t i=0;i<lent;i++){
        if (inphut[i]=='0'){
            continue;    
        }
        cout << i<< endl;
        numbir += (inphut[i]-'0')*powar(2, lent-i-1);
    }
    return numbir;
}

float placeBall(){
    // Both position and each entry in balls use the same three-coordinate
    // type, so this copies the current XYZ position into the dynamic vector.
    balls.push_back(position);
    return 1;
}

// A declaration lets callers above the function use it before its definition.
ClosestBallInfo getClosestBallInfo();

float getBall(){
    // Store the returned array locally: all values below refer to the same
    // closest-ball search, and the vector is not searched again for its index.
    const ClosestBallInfo closestBall = getClosestBallInfo();
    const float distance = closestBall[DISTANCE_INDEX];
    cout << "Distance to ball: " << distance << endl;
    // A negative distance is the "no ball" sentinel. Check it before using
    // the returned index, because an empty vector has no valid element to erase.
    if (distance >= 0 && distance <= 5){
        cout << " Got Ball"<<endl;
        // The lookup stores the vector index as a float to fit the all-float
        // result array; convert it back before using it to index the vector.
        const size_t ballIndex = static_cast<size_t>(closestBall[BALL_INDEX]);
        balls[ballIndex] = balls.back();
        balls.pop_back();
    }
    return distance;
}

float autoGetBall(){
    const ClosestBallInfo closestBall = getClosestBallInfo();
    // The lookup returns a negative distance when balls is empty. Returning
    // here prevents using missing offsets or dividing by an invalid distance.
    if (closestBall[DISTANCE_INDEX] < 0){
        cout << "No balls available to collect." << endl;
        return -1;
    }

    string line;
    cout << "How many times to move to get to ball: ";
    std::getline(std::cin,line);
    const int steps = stoi(line);
    if (steps <= 0){
        cout << "Step count must be > 0" << endl;
        return -1;
    }
    
    const float deltaY = closestBall[DELTA_Y_INDEX];
    const float deltaX = closestBall[DELTA_X_INDEX];
    const float deltaZ = closestBall[DELTA_Z_INDEX];
    const vector<float> vec = {deltaX,deltaY,deltaZ};
    const float hypotenuse =  RMS(vec);
    // If all offsets are zero, the player is already at the target. Avoid
    // dividing by this zero length while calculating the turn angle.
    if (hypotenuse == 0){
        cout << "Already at the closest ball." << endl;
        return 0;
    }
    
    const float requiredSpeed = hypotenuse/steps;
    int requiredTurn;
    
    cout << endl << "Dy, Dx, Dz " << deltaY << ", " << deltaX << ", " << deltaZ;
    
    if (deltaY<0){//
        int degree = std::asin(-deltaY/hypotenuse)*180/PI;
        cout << endl <<"degree: "<<degree<<"  ";
        if (deltaX<0){//-dx -dy
            int targetDirection = 180+degree;
            requiredTurn = targetDirection-xydirection;
        }else{//dx -dy
            int targetDirection = 350-degree;
            requiredTurn = targetDirection-xydirection;
        }
    }else{//positive player is below
        int degree = std::asin(deltaY/hypotenuse)*180/PI;
        if (deltaX<0){//-dx dy
            int targetDirection = 180-std::asin(deltaY/hypotenuse)*180/PI;
            requiredTurn = targetDirection-xydirection;
        }else{//dx dy
            requiredTurn = degree-xydirection;
        }
    }   
    if (hypotenuse !=-1){
        cout << endl << "To get to the closest ball, set speed to " << requiredSpeed << " and turn " << requiredTurn<<" 'it's "<<requiredTurn/19<<endl;
    }
    return requiredTurn;
}

ClosestBallInfo getClosestBallInfo(){
    // A distance of -1 signals that no ball was found. The other sentinel
    // values keep the whole result initialized until a candidate is selected.
    ClosestBallInfo closestBall = {-1, -1, -1, -1, -1};
    // INFINITY ensures the first real distance is smaller, including when
    // there is exactly one ball or its distance is zero.
    float shortestDistance = INFINITY;

    // Calculate distance and offsets together for each ball. When a nearer
    // ball is found, save all its values in one result; this is one O(n) scan.
    for (size_t ballIndex = 0; ballIndex < balls.size(); ++ballIndex){
        const float deltaX = balls[ballIndex][0] - position[0];
        const float deltaY = balls[ballIndex][1] - position[1];
        const float deltaZ = balls[ballIndex][2] - position[2];
        const vector<float> veco = {deltaX,deltaY,deltaZ};
        const float distance = RMS(veco);
        //sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);

        if (distance < shortestDistance){
            shortestDistance = distance;
            // Aggregate initialization fills the result in the documented
            // order above, with an explicit conversion for the vector index.
            closestBall = {
                distance,
                deltaX,
                deltaY,
                deltaZ,
                static_cast<float>(ballIndex)
            };
        }
    }

    // Returning std::array by value gives the caller an independent,
    // fixed-size copy of the result (small arrays like this are inexpensive).
    return closestBall;
}
float move(){
    //cout << "why aren't we moving"<<endl;
    float radi = -xydirection*PI/180;
    float gadi = -xzdirection*PI/180;
    const vector<float> vecc = {1,std::tan(radi),std::tan(gadi)};
    float xM= normalMovement/RMS(vecc);
    float yM=xM*std::tan(radi);
    float zM=xM*std::tan(gadi);
    float t1 = position[0]+round(100.0*xM)/100.0;
    float t2 = position[1]+round(100.0*yM)/100.0;
    float t3 = position[2]+round(100.0*zM)/100.0;
    if (std::abs(t1)>= radius){
        t1 = radius*t1/std::abs(t1)-0.01f;
        cout <<endl<< "Caution: Hit Wall X"<<endl<<endl;
    }
    if (std::abs(t2)>=radius){
        t2 = radius*t2/std::abs(t2)-0.01f;
        cout <<endl<<"Caution: Hit Wall Y"<<endl<<endl;
    }
    if (std::abs(t3)>=radius){
        t3 = radius*t3/std::abs(t3)-0.01f;
        cout <<endl<<"Caution: Hit Wall Z"<<endl<<endl;
        
    }
    position[0] = t1;
    position[1] = t2;
    position[2] = t3;
    return 0;
}

/*Bak adam bize diyecek ki 009A donmek istiyorum
Biz bunu 0x19+0 xz yonunde 0,   9x19+10 181 xy yonunde donmek
istiyourm diye anliyacagiz*/

int giveNona(string binput){
    //binput here is B9 or JJ type 2 digits
    char rep[9]= {'A','B','C','D','E','F','G','H','I'};
    int nums[9] = {10 ,11 ,12 ,13 ,14 ,15 ,16 ,17 ,18};
    int convertedDegrees = 0;
    if (std::isdigit(binput[0])){
        convertedDegrees += (binput[0]-'0')*19;
    }if(std::isdigit(binput[1])){
        convertedDegrees += (binput[1]-'0');
    }
    for (int i=0;i<9;i++){
        if (binput[0] == rep[i]){
            convertedDegrees += nums[i]*19;
        }
        if (binput[1] == rep[i]){
            convertedDegrees += nums[i];
        }
    }
    return convertedDegrees;
}

int turn(string degres){
    //cout << "Enter amount in degrees in nonadecimal to turn: ";
    //std::getline(std::cin,degres);
    int xzdegre = 0;
    int xydegre = 0;
    //Now 
    std::transform(degres.begin(),degres.end(),degres.begin(),[](unsigned char c){return std::toupper(c);});
    if (degres.length()>=2){
        string xyx = degres.substr(0, 2);
        xydegre = giveNona(xyx);
        if (degres.length()>=4){
            xzdegre = xydegre;
            string xzx = degres.substr(2, 2);
            xydegre = giveNona(xzx);
        }
    }
    
    xydirection += xydegre;
    xydirection %= 360;
    
    xzdirection +=xzdegre;
    xzdirection %= 360;
    return 0;
}

float setSpeed(string newSpeed){
    float num = normalMovement;
    try {
        num = toBinary(newSpeed);
        cout << num<<endl;
    }catch(std::invalid_argument){
        cout << endl<< endl<<"Invalid Input"<<endl<<endl;
    }
    normalMovement = num;
    return 0;
}

float getDistance();

int giveInfo(){
    cout << endl << "You are at: "<<position[0]<<", "<<position[1]<<", "<<position[2]<< endl<< "xydirection: "<< xydirection<< " Degrees"<< endl<< "xzdirection: "<< xzdirection<< " Degrees"<< endl << "Speed: " << normalMovement<< endl << "Distance from origin: " << 0.01*round(100*getDistance());
    // This display only needs the distance, so it reads that field from the
    // same result type used by getBall and autoGetBall.
    cout << endl << "Distance to closest Ball: " << getClosestBallInfo()[DISTANCE_INDEX]<<endl<<"Number of balls: "<<balls.size()<<endl<< endl;
    return 0;
}
string checkCommand(string cmd);
float getDistance(){
    float n1 = position[0];
    float n2 = position[1];
    float n3 = position[2];
    const vector<float> vecco = {n1,n2,n3};
    return RMS(vecco);
}
string nextCommand(string gonogo){
    string input;
    if(gonogo.length()<2){
        std::getline(std::cin, input);
    }else{
        input = gonogo;   
    }
    bool end = 0;
    int spec = 0;
    string specComs;
    //So I will be adding a way to include nonadecimal inputs for the speed and turning commands
    //To do this the input is going to be standardized into being between paranthesis 4 digits(which makes it 6 digits or 0 mod 3)
    //If it's inside a function it still doesn't matter because the command is still 3x characters long
    //but the evaluation is different when it is within a function,
    //let's first handle the normal non-function version

    //I've changed the input for speed and turn commands, and they will be executed directly from here
    //Also thinking about executing every command from here actually

    /*
    Input:
    000001(00EE)011
    becomes 000, 001(00EE), 011
    001(00EE)
    make 001 wait then send it together when spec = 2
    2nd Input:
    fun000001(004e)fun
    end = 1
    not func, adding to funcs
    not func, adding to funcs
    not func, adding to funcs
    not func, adding to funcs
    end = 0
    */
    vector<string> funcs;
    for (size_t ii=0; ii<input.length()/3;ii++){
        string commaned;
        if ((spec == 0 && end == 0) || end == 1){
        commaned += input[ii*3];
        commaned += input[ii*3+1];
        commaned += input[ii*3+2];
        }else if(spec == 1 || spec == 11){
            specComs += input[ii*3+1];
            specComs += input[ii*3+2];
            spec += 1;
        }else if(spec == 2 || spec == 12){
            specComs += input[ii*3];
            specComs += input[ii*3+1];
            if (spec == 2 && end == 0){
                turn(specComs);
            }else if(end == 0){
                setSpeed(specComs);
            }
            specComs = "";
            spec = 0;
        }
        if (spec == 0){
            if (commaned == "001"){
                spec = 1;
            }else if(commaned == "010"){
                spec = 11;
            }
        }
        //This creates functions with commands in between "fun" commands
        //The user inputs another vector with names of functions, uses those names to call the functions
        if (end == 1){//Inside the function
            if (commaned == "fun"){//closing the function
                end=0;
                functions.push_back(funcs);
                string nameOfFunc;
                //can be named as one of the original functions and mess up slightly
                cout<<"Enter 3 character function name, should be different than the original functions: ";
                std::getline(std::cin, nameOfFunc);
                functionen.push_back(nameOfFunc);
                for (string comend : funcs){
                    cout << comend << endl;
                }
                funcs.clear();
            }else{//lines of the function
                funcs.push_back(commaned);    
            }
        }else{//not inside function
            if(commaned == "fun"){//opening the function
                end = 1;
            }else{//calling a function/plain
                size_t iters = 0;
                bool found = false;
                for (string funky: functionen){//Calling a new function
                    if (commaned == funky){
                        found = true;
                        string fullCommand;
                        for(string cmd: functions[iters]){
                            fullCommand += cmd;
                            //make the commands of the function a string
                        }
                        nextCommand(fullCommand);
                        //executes the new function
                        break;
                    }
                    iters ++;
                }
                if (found == false){//plain--Spec functions && (commaned != "001" && commaned != "010")
                    checkCommand(commaned);
                }
            }
        }
    }
    return input;
}
string checkCommand(string cmd){
    string input = "";
    if (cmd == "000"){
        move();    
    }
    if (cmd == "011"){
        giveInfo();
    }
    if (cmd == "100"){
        getBall();
    }
    if (cmd=="101"){
        placeBall();    
    }
    if (cmd == "110"){
        autoGetBall();
    }
    if (cmd == "cmd"){
        cout << "current functions: . . ";
        for (string funky : functionen){
            cout << funky<<" . . ";    
        }
        cout << endl;
    }
    if (cmd == "111" && lastCommand!="111"){
        checkCommand(lastCommand);
    }
    lastCommand = cmd;
    return input;
}
//00 JJ==360 Degrees
//A B C D E F G H I J 
//0 1 2 3 4 5 6 7 8 9
int main()
{
    cout << "Instructions:"<<endl<< "000 to move"<< endl<<"001 to turn"<<endl<<"010 to set speed"<<endl<<"011 to display current status"<< endl<< "100 to get ball"<<endl<<"101 to place ball"<<endl<< "110 to auto get ball"<<endl<<endl<< "Turning is a 4 digit nonadecimal or base-19 code. The first 2 digits are turning in the xz plane, the second 2 digits are for turning in the xy plane A(10) B C D E F G H I(18)"<<endl<<endl<< "Use ..fun.. to create a command then enter the name for the command.\nReminder: don't call the command you created in the command\n";
    cout << "90 degrees is 4E, 180 is 99, 270 is E4, 360 is II(max 19^2-1)"<<endl;
    giveInfo();
    string line;
    while (true){
        cout << "Enter next command: ";
        line = nextCommand("1");
    }
    return 0;
}