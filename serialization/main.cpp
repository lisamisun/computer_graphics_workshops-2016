#include <iostream>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <ctime>

#include "Utility.h"
#include <SOIL.h>

using namespace std;

const uint GRASS_INSTANCES = 10000; // Количество травинок

GL::Camera camera;               // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                                 // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                                 // Вы можете просто пользоваться этим классом для расчёта указанных матриц.


GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassShader;      // Шейдер, рисующий траву
GLuint grassVAO;         // VAO для травы (что такое VAO почитайте в доках)
GLuint grassTexture;     // текстура для травы

// теперь можно переприбивать буферы!
// количество травинок везде одинаковое
GLuint scaleBuffer; // буфер для масштаба травинок
vector<float> grassScale(GRASS_INSTANCES); // масштаб травинок
GLuint turningBuffer; // буфер для угла поворота травинок
vector<float> grassTurning(GRASS_INSTANCES); // углы поворота травинок
GLuint colorBuffer; // буфер для номеров цветов травинок
vector<float> grassColors(GRASS_INSTANCES); // цвета травинок
GLuint positionBuffer; // буфер для позиций травинок
vector<VM::vec2> grassPositions(GRASS_INSTANCES); // вектор с позициями травинок

// параметры ветра
GLuint grassVariance;    // Буфер для смещения координат травинок
vector<VM::vec4> grassVarianceData(GRASS_INSTANCES); // Вектор со смещениями для координат травинок

float max_amplitude; // максимальное отклонение травы
float amplitude; // амплитуда колебания травы
float stepToMax, stepToMin; // шаг увеличения/уменьшения амплитуды колебаний
float cyclicFrequency; // циклическая частота колебаний травы
clock_t startedTimer = clock(); // таймер
double t = 0; // время для функции ветра
double timeForWind = ((double)rand()) / RAND_MAX * 0.4 + 0.1; // ветер уже пора?;
int winding = 0; // ветер дует?

GLuint groundShader; // Шейдер для земли
GLuint groundVAO; // VAO для земли
GLuint groundTexture; // текстура для земли

// Размеры экрана
uint screenWidth = 800;
uint screenHeight = 600;

// Это для захвата мышки. Вам это не потребуется (это не значит, что нужно удалять эту строку)
bool captureMouse = true;


// Сериализация
// абстрактный класс сериализации
class ISerializer
{
public:
    virtual void InOut(vector<float> &savingVecF) = 0;
    virtual void InOut(vector<VM::vec2> &savingVecV2) = 0;
    virtual void InOut(vector<VM::vec4> &savingVecV4) = 0;
    virtual void InOut(float &savingF) = 0;
    virtual void InOut(int &savingI) = 0;
    virtual void InOut(double &savingD) = 0;
};

// сериализация в бинарный файл - определяет, откуда и как читать или записывать
class BinarySerializerReader: public ISerializer
{
    ifstream file;
public:
    BinarySerializerReader(const string &inName); // конструктор - открываем файл на чтение

    // доопределяем виртуальные функции для чтения
    virtual void InOut(vector<float> &savingVecF);
    virtual void InOut(vector<VM::vec2> &savingVecV2);
    virtual void InOut(vector<VM::vec4> &savingVecV4);
    virtual void InOut(float &savingF);
    virtual void InOut(int &savingI);
    virtual void InOut(double &savingD);

    ~BinarySerializerReader(); // деструктор - закрываем файл
};

class BinarySerializerWriter: public ISerializer
{
    ofstream file;
public:
    BinarySerializerWriter(const string &fromName); // конструктор

    // доопределяем виртуальные функции для записи
    virtual void InOut(vector<float> &savingVecF);
    virtual void InOut(vector<VM::vec2> &savingVecV2);
    virtual void InOut(vector<VM::vec4> &savingVecV4);
    virtual void InOut(float &savingF);
    virtual void InOut(int &savingI);
    virtual void InOut(double &savingD);

    ~BinarySerializerWriter(); // деструктор
};

// сериализация в текстовый файл - определяет, откуда и как читать или записывать
class TextSerializerReader: public ISerializer
{
    ifstream file;
public:
    TextSerializerReader(const string &inName); // конструктор

    // доопределяем виртуальные функции для чтения
    virtual void InOut(vector<float> &savingVecF);
    virtual void InOut(vector<VM::vec2> &savingVecV2);
    virtual void InOut(vector<VM::vec4> &savingVecV4);
    virtual void InOut(float &savingF);
    virtual void InOut(int &savingI);
    virtual void InOut(double &savingD);

    ~TextSerializerReader();
};

class TextSerializerWriter: public ISerializer
{
    ofstream file;
public:
    TextSerializerWriter(const string &fromName); // конструктор

    // доопределяем виртуальные функции для записи
    virtual void InOut(vector<float> &savingVecF);
    virtual void InOut(vector<VM::vec2> &savingVecV2);
    virtual void InOut(vector<VM::vec4> &savingVecV4);
    virtual void InOut(float &savingF);
    virtual void InOut(int &savingI);
    virtual void InOut(double &savingD);

    ~TextSerializerWriter();
};

// класс хранимых настроек - определяет, что записывать или читать
class SavingSettings
{
public:
    vector<float> grassScale;
    vector<float> grassTurning;
    vector<float> grassColors;
    vector<VM::vec2> grassPositions;

    vector<VM::vec4> grassVarianceData; // сохраняем текущее смещение травы

    // сохраняем все настройки для ветра
    double timeForWind;
    double t;
    int winding;
    float max_amplitude;
    float amplitude;
    float stepToMax;
    float stepToMin;
    float cyclicFrequency;

    void Serialize(ISerializer &inOut);
};

// реализация методов записи в бинарный файл
TextSerializerReader::TextSerializerReader(const string &inName) {
    file.open(inName);
}

void TextSerializerReader::InOut(vector<float> &savingVecF){
    float buffer;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file >> buffer;
        savingVecF.push_back(buffer);
    }
}

void TextSerializerReader::InOut(vector<VM::vec2> &savingVecV2){
    float bufferX, bufferY;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file >> bufferX;
        file >> bufferY;
        savingVecV2.push_back(VM::vec2(bufferX, bufferY));
    }
}

void TextSerializerReader::InOut(vector<VM::vec4> &savingVecV4){
    float bufferX, bufferY, bufferZ, bufferW;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file >> bufferX;
        file >> bufferY;
        file >> bufferZ;
        file >> bufferW;
        savingVecV4.push_back(VM::vec4(bufferX, bufferY, bufferZ, bufferW));
    }
}

void TextSerializerReader::InOut(float &savingF){
    file >> savingF;
}

void TextSerializerReader::InOut(double &savingD){
    file >> savingD;
}

void TextSerializerReader::InOut(int &savingI){
    file >> savingI;
}

TextSerializerReader::~TextSerializerReader() {
    file.close();
}

// реализация методов чтения из бинарного файла
BinarySerializerReader::BinarySerializerReader(const string &inName) {
    file.open(inName, ios::binary);
}

void BinarySerializerReader::InOut(vector<float> &savingVecF){
    float buffer;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.read((char *) &buffer, sizeof(buffer));
        savingVecF.push_back(buffer);
    }
}

void BinarySerializerReader::InOut(vector<VM::vec2> &savingVecV2){
    float bufferX, bufferY;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.read((char *) &bufferX, sizeof(bufferX));
        file.read((char *) &bufferY, sizeof(bufferY));
        savingVecV2.push_back(VM::vec2(bufferX, bufferY));
    }
}

void BinarySerializerReader::InOut(vector<VM::vec4> &savingVecV4){
    float bufferX, bufferY, bufferZ, bufferW;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.read((char *) &bufferX, sizeof(bufferX));
        file.read((char *) &bufferY, sizeof(bufferY));
        file.read((char *) &bufferZ, sizeof(bufferZ));
        file.read((char *) &bufferW, sizeof(bufferW));
        savingVecV4.push_back(VM::vec4(bufferX, bufferY, bufferZ, bufferW));
    }
}

void BinarySerializerReader::InOut(float &savingF){
    file.read((char *) &savingF, sizeof(savingF));
}

void BinarySerializerReader::InOut(double &savingD){
    file.read((char *) &savingD, sizeof(savingD));
}

void BinarySerializerReader::InOut(int &savingI){
    file.read((char *) &savingI, sizeof(savingI));
}

BinarySerializerReader::~BinarySerializerReader() {
    file.close();
}

// реализация методов зписи в текстовый файл
TextSerializerWriter::TextSerializerWriter(const string &inName) {
    file.open(inName);
}

void TextSerializerWriter::InOut(vector<float> &savingVecF){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file << savingVecF[i] << ' ';
    }
}

void TextSerializerWriter::InOut(vector<VM::vec2> &savingVecV2){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file << savingVecV2[i].x << ' ';
        file << savingVecV2[i].y << ' ';
    }
}

void TextSerializerWriter::InOut(vector<VM::vec4> &savingVecV4){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file << savingVecV4[i].x << ' ';
        file << savingVecV4[i].y << ' ';
        file << savingVecV4[i].z << ' ';
        file << savingVecV4[i].w << ' ';
    }
}

void TextSerializerWriter::InOut(float &savingF){
    file << savingF << ' ';
}

void TextSerializerWriter::InOut(double &savingD){
    file << savingD << ' ';
}

void TextSerializerWriter::InOut(int &savingI){
    file << savingI << ' ';
}

TextSerializerWriter::~TextSerializerWriter() {
    file.close();
}

// реализация методов зписи в бинарный файл
BinarySerializerWriter::BinarySerializerWriter(const string &inName) {
    file.open(inName, ios::binary);
}

void BinarySerializerWriter::InOut(vector<float> &savingVecF){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.write((char *) &savingVecF[i], sizeof(savingVecF[i]));
    }
}

void BinarySerializerWriter::InOut(vector<VM::vec2> &savingVecV2){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.write((char *) &savingVecV2[i].x, sizeof(savingVecV2[i].x));
        file.write((char *) &savingVecV2[i].y, sizeof(savingVecV2[i].y));
    }
}

void BinarySerializerWriter::InOut(vector<VM::vec4> &savingVecV4){
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        file.write((char *) &savingVecV4[i].x, sizeof(savingVecV4[i].x));
        file.write((char *) &savingVecV4[i].y, sizeof(savingVecV4[i].y));
        file.write((char *) &savingVecV4[i].z, sizeof(savingVecV4[i].z));
        file.write((char *) &savingVecV4[i].w, sizeof(savingVecV4[i].w));
    }
}

void BinarySerializerWriter::InOut(float &savingF){
    file.write((char *) &savingF, sizeof(savingF));
}

void BinarySerializerWriter::InOut(double &savingD){
    file.write((char *) &savingD, sizeof(savingD));
}

void BinarySerializerWriter::InOut(int &savingI){
    file.write((char *) &savingI, sizeof(savingI));
}

BinarySerializerWriter::~BinarySerializerWriter() {
    file.close();
}


void SavingSettings::Serialize(ISerializer &inOut) {
    // общие настройки травы
    inOut.InOut(grassScale); // сохраняем масштаб травинок
    inOut.InOut(grassTurning); // сохраняем повороты травинок
    inOut.InOut(grassColors); // сохраняем цвета транвинок
    inOut.InOut(grassPositions); // сохраняем позиции травинок

    // настройки ветра
    inOut.InOut(grassVarianceData); // сохраняем текущее смещение травы

    inOut.InOut(timeForWind); // сохраняем все настройки для ветра
    inOut.InOut(t);
    inOut.InOut(winding);
    inOut.InOut(max_amplitude);
    inOut.InOut(amplitude);
    inOut.InOut(stepToMax);
    inOut.InOut(stepToMin);
    inOut.InOut(cyclicFrequency);
}


// Функция, рисующая замлю
void DrawGround() {
    // Используем шейдер для земли
    glUseProgram(groundShader);                                                  CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(groundShader, "camera");         CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?)                                                     // ###
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    // подключаем текстуру
    glBindTexture(GL_TEXTURE_2D, groundTexture);                                 CHECK_GL_ERRORS

    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    // Рисуем землю: 2 треугольника (6 вершин)
    glDrawArrays(GL_TRIANGLES, 0, 6);                                            CHECK_GL_ERRORS

    // отключаем текстуру
    glBindTexture(GL_TEXTURE_2D, 0);                                             CHECK_GL_ERRORS
    // Отсоединяем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Обновление смещения травинок
void UpdateGrassVariance() {
    // Генерация случайных смещений
    t += (clock() - startedTimer) / (double)CLOCKS_PER_SEC; // секунды c момента начала таймера

    if ((winding) || ((!winding) && (t > timeForWind))) { // если уже ветер или пора ветру дуть
        if (!winding) { // ветру пора дуть
            winding = 1;
            max_amplitude = ((double)rand()) / RAND_MAX * 1.5 + 1; // сила ветра
            stepToMax = max_amplitude / 50.0;
            stepToMin = max_amplitude / 500.0;
            amplitude = 0;
            cyclicFrequency = max_amplitude * 50;

            t = 0; // обновляется время для ветра
        } else { // ветер уже дует
            if (max_amplitude != 0) {
                amplitude += stepToMax; // постепенно увеличиваем амплитуду на каждой итерации
                if (amplitude >= max_amplitude) {
                    max_amplitude = 0; // теперь амплитуду нужно уменьшать
                }
            } else {
                amplitude -= stepToMin; // уменьшаем амплитуду на каждой итерации
            }

            if (amplitude <= 0) {
                max_amplitude = amplitude = 0; // колебания травы затухли
                winding = 0;
                timeForWind = ((double)rand()) / RAND_MAX * 0.4 + 0.1; // ветер уже пора?

                t = 0; // обновляется время для ожидания
            }
        }

        for (uint i = 0; i < GRASS_INSTANCES; ++i) {
            grassVarianceData[i].x = amplitude * sin(cyclicFrequency * t); // по x изменяемся синусом
            // y и z в силу обстоятельств меняем в шейдере
        }
    } // иначе смещение == 0

    startedTimer = clock(); // обновляем таймер

    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}


// Рисование травы
void DrawGrass() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    // подключаем текстуру
    glBindTexture(GL_TEXTURE_2D, grassTexture);                                 CHECK_GL_ERRORS

    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS
    // Обновляем смещения для травы
    UpdateGrassVariance(); // временно убираем
    // Отрисовка травинок в количестве GRASS_INSTANCES
    glDrawArraysInstanced(GL_TRIANGLES, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    glEnable(GL_MULTISAMPLE_ARB);
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST);
    // Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Рисуем меши
    DrawGrass();
    DrawGround();
    glutSwapBuffers();
}

// Завершение программы
void FinishProgram() {
    glutDestroyWindow(glutGetWindow());
}

// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
    if (key == 27) {
        FinishProgram();
    } else if (key == 'w') {
        camera.goForward();
    } else if (key == 's') {
        camera.goBack();
    } else if (key == 'm') {
        captureMouse = !captureMouse;
        if (captureMouse) {
            glutWarpPointer(screenWidth / 2, screenHeight / 2);
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
        }
    } else if (key == 'k') {
        SavingSettings settings;
        settings.grassScale = grassScale;
        settings.grassTurning = grassTurning;
        settings.grassColors = grassColors;
        settings.grassPositions = grassPositions;

        settings.grassVarianceData = grassVarianceData;

        settings.timeForWind = timeForWind;
        settings.t = t;
        settings.winding = winding;
        settings.max_amplitude = max_amplitude;
        settings.amplitude = amplitude;
        settings.stepToMax = stepToMax;
        settings.stepToMin = stepToMin;
        settings.cyclicFrequency = cyclicFrequency;

        TextSerializerWriter textWriter("../Settings/settings.txt");
        settings.Serialize(textWriter);

        startedTimer = clock();

    } else if (key == 'i') {
        SavingSettings settings;
        settings.grassScale = grassScale;
        settings.grassTurning = grassTurning;
        settings.grassColors = grassColors;
        settings.grassPositions = grassPositions;

        settings.grassVarianceData = grassVarianceData;

        settings.timeForWind = timeForWind;
        settings.t = t;
        settings.winding = winding;
        settings.max_amplitude = max_amplitude;
        settings.amplitude = amplitude;
        settings.stepToMax = stepToMax;
        settings.stepToMin = stepToMin;
        settings.cyclicFrequency = cyclicFrequency;

        BinarySerializerWriter binaryWriter("../Settings/settings.bin");
        settings.Serialize(binaryWriter);

        startedTimer = clock();

    } else if (key == 'l') {

        SavingSettings settings;

        TextSerializerReader textReader("../Settings/settings.txt");
        settings.Serialize(textReader);

        grassScale = settings.grassScale;
        grassTurning = settings.grassTurning;
        grassColors = settings.grassColors;
        grassPositions = settings.grassPositions;

        grassVarianceData = settings.grassVarianceData;

        timeForWind = settings.timeForWind;
        t = settings.t;
        winding = settings.winding;
        max_amplitude = settings.max_amplitude;
        amplitude = settings.amplitude;
        stepToMax = settings.stepToMax;
        stepToMin = settings.stepToMin;
        cyclicFrequency = settings.cyclicFrequency;

        glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassScale.size(), grassScale.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, turningBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassTurning.size(), grassTurning.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassColors.size(), grassColors.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS

        startedTimer = clock();

    } else if (key == 'o') {
        SavingSettings settings;

        BinarySerializerReader binaryReader("../Settings/settings.bin");
        settings.Serialize(binaryReader);

        grassScale = settings.grassScale;
        grassTurning = settings.grassTurning;
        grassColors = settings.grassColors;
        grassPositions = settings.grassPositions;

        grassVarianceData = settings.grassVarianceData;

        timeForWind = settings.timeForWind;
        t = settings.t;
        winding = settings.winding;
        max_amplitude = settings.max_amplitude;
        amplitude = settings.amplitude;
        stepToMax = settings.stepToMax;
        stepToMin = settings.stepToMin;
        cyclicFrequency = settings.cyclicFrequency;

        glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassScale.size(), grassScale.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, turningBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassTurning.size(), grassTurning.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassColors.size(), grassColors.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                         CHECK_GL_ERRORS

        glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
        glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
        glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS

        startedTimer = clock();

    }
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT) {
        camera.rotateY(0.02);
    } else if (key == GLUT_KEY_LEFT) {
        camera.rotateY(-0.02);
    } else if (key == GLUT_KEY_UP) {
        camera.rotateTop(-0.02);
    } else if (key == GLUT_KEY_DOWN) {
        camera.rotateTop(0.02);
    }
}

void IdleFunc() {
    glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {
    if (captureMouse) {
        int centerX = screenWidth / 2,
            centerY = screenHeight / 2;
        if (x != centerX || y != centerY) {
            camera.rotateY((x - centerX) / 1000.0f);
            camera.rotateTop((y - centerY) / 1000.0f);
            glutWarpPointer(centerX, centerY);
        }
    }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
}

// Событие изменение размера окна
void windowReshapeFunc(GLint newWidth, GLint newHeight) {
    glViewport(0, 0, newWidth, newHeight);
    screenWidth = newWidth;
    screenHeight = newHeight;

    camera.screenRatio = (float)screenWidth / screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitContextVersion(3, 0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Computer Graphics 3");
    glutWarpPointer(400, 300);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(RenderLayouts);
    glutKeyboardFunc(KeyboardEvents);
    glutSpecialFunc(SpecialButtons);
    glutIdleFunc(IdleFunc);
    glutPassiveMotionFunc(MouseMove);
    glutMouseFunc(MouseClick);
    glutReshapeFunc(windowReshapeFunc);

    glDisable(GL_MULTISAMPLE_ARB);
}

// Генерация позиций травинок
vector<VM::vec2> GenerateGrassPositions() {
    vector<VM::vec2> grassPositions(GRASS_INSTANCES);
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        // это размещение по равномерной сетке
        grassPositions[i] = VM::vec2((i % 100) / 100.0, (i / 100) / 100.0) + VM::vec2(1, 1) / 200;
        // это смещение относительно позиции точки в сетке
        grassPositions[i] += VM::vec2(((double)(rand()) / RAND_MAX - 0.5) / 200 , ((double)(rand()) / RAND_MAX - 0.5) / 200);
    }
    return grassPositions;
}

// генерация масштаба травинки
vector<float> GenerateGrassScales() {
    vector<float> grassScales(GRASS_INSTANCES);
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        grassScales[i] = (rand() % 8 + 3) / 100.0f; // генерируем случайное вещественное число в диапазоне 0.03..0.1 с шагом 0.01
        //grassScales[i] = (double)(rand()) / RAND_MAX * 0.1 + 0.03; // или так
    }
    return grassScales;
}

// генерация угла поворота травинки
vector<float> GenerateGrassTurning() {
    vector<float> grassTurning(GRASS_INSTANCES);
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        grassTurning[i] = (double)(rand()) / RAND_MAX * 2 * M_PI;
    }
    return grassTurning;
}

vector<float> GenerateGrassColors() {
    vector<float> grassColors(GRASS_INSTANCES);
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
        if ((rand() % 50) == 0) {
            grassColors[i] = 4;
        } else {
            grassColors[i] = rand() % 3 + 1;
        }
    }
    return grassColors;
}

// Здесь вам нужно будет генерировать меш
vector<VM::vec4> GenMesh(uint n) {
    return {
        VM::vec4(0, 0, 0, 1), // правый нижний прямоугольник
        VM::vec4(0.5, 0, -0.4, 1),
        VM::vec4(0.3, 0.55, -0.3, 1),

        VM::vec4(0, 0, 0, 1),
        VM::vec4(0.3, 0.55, -0.3, 1),
        VM::vec4(0, 0.6, 0, 1),

        VM::vec4(0, 0, 0, 1), // левый нижний прямоугольник
        VM::vec4(-0.5, 0, -0.4, 1),
        VM::vec4(-0.3, 0.55, -0.3, 1),

        VM::vec4(0, 0, 0, 1),
        VM::vec4(-0.3, 0.55, -0.3, 1),
        VM::vec4(0, 0.6, 0, 1),

        VM::vec4(0, 1, -0.5, 1), // правый верхний треугольник
        VM::vec4(0.3, 0.55, -0.3, 1),
        VM::vec4(0, 0.6, 0, 1),

        VM::vec4(0, 1, -0.5, 1), // левый верхний треугольник
        VM::vec4(-0.3, 0.55, -0.3, 1),
        VM::vec4(0, 0.6, 0, 1)
    };
}

// цвет 1
vector<VM::vec3> GenGreen1() {
    return {
        VM::vec3(0.5, 0.5, 0), // правый нижний прямоугольник
        VM::vec3(0.5, 0.5, 0),
        VM::vec3(0.4, 0.6, 0.2),

        VM::vec3(0.5, 0.5, 0),
        VM::vec3(0.4, 0.6, 0.2),
        VM::vec3(0.4, 0.6, 0.2),

        VM::vec3(0.5, 0.5, 0), // левый нижний прямоугольник
        VM::vec3(0.5, 0.5, 0),
        VM::vec3(0.4, 0.6, 0.2),

        VM::vec3(0.5, 0.5, 0),
        VM::vec3(0.4, 0.6, 0.2),
        VM::vec3(0.4, 0.6, 0.2),

        VM::vec3(0.4, 0.8, 0.2), // правый верхний треугольник
        VM::vec3(0.4, 0.6, 0.2),
        VM::vec3(0.4, 0.6, 0.2),

        VM::vec3(0.4, 0.8, 0.2), // левый верхний треугольник
        VM::vec3(0.4, 0.6, 0.2),
        VM::vec3(0.4, 0.6, 0.2)
    };
}

// цвет 2
vector<VM::vec3> GenGreen2() {
    return {
        VM::vec3(0.2, 0.2, 0), // правый нижний прямоугольник
        VM::vec3(0.2, 0.2, 0),
        VM::vec3(0.2, 0.6, 0.2),

        VM::vec3(0.2, 0.2, 0),
        VM::vec3(0.2, 0.6, 0.2),
        VM::vec3(0.2, 0.6, 0.2),

        VM::vec3(0.2, 0.2, 0), // левый нижний прямоугольник
        VM::vec3(0.2, 0.2, 0),
        VM::vec3(0.2, 0.6, 0.2),

        VM::vec3(0.2, 0.2, 0),
        VM::vec3(0.2, 0.6, 0.2),
        VM::vec3(0.2, 0.6, 0.2),

        VM::vec3(0.2, 0.8, 0.2), // правый верхний треугольник
        VM::vec3(0.2, 0.6, 0.2),
        VM::vec3(0.2, 0.6, 0.2),

        VM::vec3(0.2, 0.8, 0.2), // левый верхний треугольник
        VM::vec3(0.2, 0.6, 0.2),
        VM::vec3(0.2, 0.6, 0.2)
    };
}

// цвет 3
vector<VM::vec3> GenGreen3() {
    return {
        VM::vec3(0, 0.2, 0), // правый нижний прямоугольник
        VM::vec3(0, 0.2, 0),
        VM::vec3(0, 0.6, 0.2),

        VM::vec3(0, 0.2, 0),
        VM::vec3(0, 0.6, 0.2),
        VM::vec3(0, 0.6, 0.2),

        VM::vec3(0, 0.2, 0), // левый нижний прямоугольник
        VM::vec3(0, 0.2, 0),
        VM::vec3(0, 0.6, 0.2),

        VM::vec3(0, 0.2, 0),
        VM::vec3(0, 0.6, 0.2),
        VM::vec3(0, 0.6, 0.2),

        VM::vec3(0, 0.8, 0.2), // правый верхний треугольник
        VM::vec3(0, 0.6, 0.2),
        VM::vec3(0, 0.6, 0.2),

        VM::vec3(0, 0.8, 0.2), // левый верхний треугольник
        VM::vec3(0, 0.6, 0.2),
        VM::vec3(0, 0.6, 0.2)
    };
}

// цвет 4 - жёлтый
// цвет 4 - жёлтый
vector<VM::vec3> GenYellow() {
    return {
        VM::vec3(0.6, 0.6, 0), // правый нижний прямоугольник
        VM::vec3(0.6, 0.6, 0),
        VM::vec3(0.6, 0.6, 0.2),

        VM::vec3(0.6, 0.6, 0),
        VM::vec3(0.6, 0.6, 0.2),
        VM::vec3(0.6, 0.6, 0.2),

        VM::vec3(0.6, 0.6, 0), // левый нижний прямоугольник
        VM::vec3(0.6, 0.6, 0),
        VM::vec3(0.6, 0.6, 0.2),

        VM::vec3(0.6, 0.6, 0),
        VM::vec3(0.6, 0.6, 0.2),
        VM::vec3(0.6, 0.6, 0.2),

        VM::vec3(0.8, 0.8, 0.2), // правый верхний треугольник
        VM::vec3(0.6, 0.6, 0.2),
        VM::vec3(0.6, 0.6, 0.2),

        VM::vec3(0.8, 0.8, 0.2), // левый верхний треугольник
        VM::vec3(0.6, 0.6, 0.2),
        VM::vec3(0.6, 0.6, 0.2)
    };
}

// координаты текстуры
vector<VM::vec2> GenTexCoords() {
    return {
        VM::vec2(0.5, 0), // правый нижний прямоугольник
        VM::vec2(1, 0),
        VM::vec2(1, 0.55),

        VM::vec2(0.5, 0),
        VM::vec2(1, 0.55),
        VM::vec2(0.5, 0.6),

        VM::vec2(0.5, 0), // левый нижний прямоугольник
        VM::vec2(0, 0),
        VM::vec2(0, 0.55),

        VM::vec2(0, 0),
        VM::vec2(0, 0.55),
        VM::vec2(0.5, 0.6),

        VM::vec2(0.5, 1), // правый верхний треугольник
        VM::vec2(1, 0.55),
        VM::vec2(0.5, 0.6),

        VM::vec2(0.5, 1), // левый верхний треугольник
        VM::vec2(0, 0.55),
        VM::vec2(0.5, 0.6),
    };
}

// Создание травы
void CreateGrass() {
    uint LOD = 1;
    // Создаём меш
    vector<VM::vec4> grassPoints = GenMesh(LOD);
    // Сохраняем количество вершин в меше травы
    grassPointsCount = grassPoints.size();
    // координаты текстуры
    vector<VM::vec2> texCoords = GenTexCoords();
    // Создаём позиции для травинок
    grassPositions = GenerateGrassPositions();
    // Инициализация смещений для травинок
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassVarianceData[i] = VM::vec4(0, 0, 0, 0);
    }
    // создаем масштаб для шейдера
    grassScale = GenerateGrassScales();
    // создаем угол поворота для шейдера
    grassTurning = GenerateGrassTurning();
    // создаём номера цвета для шейдера
    grassColors = GenerateGrassColors();
    // создаём цвета для каждой вершины
    vector<VM::vec3> colorGreen1 = GenGreen1();
    vector<VM::vec3> colorGreen2 = GenGreen2();
    vector<VM::vec3> colorGreen3 = GenGreen3();
    vector<VM::vec3> colorYellow = GenYellow();

    /* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует.
    */
    grassShader = GL::CompileShaderProgram("grass");

    // Здесь создаём буфер
    GLuint pointsBuffer;
    // Это генерация одного буфера (в pointsBuffer хранится идентификатор буфера)
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    // Привязываем сгенерированный буфер
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    // Заполняем буфер данными из вектора
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    // Создание VAO
    // Генерация VAO
    glGenVertexArrays(1, &grassVAO);                                             CHECK_GL_ERRORS
    // Привязка VAO
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS

    // Получение локации параметра 'point' в шейдере
    GLuint pointsLocation = glGetAttribLocation(grassShader, "point");           CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);          CHECK_GL_ERRORS

    // Создаём буфер для координат текстур
    GLuint texCoordsBuffer;
    glGenBuffers(1, &texCoordsBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, texCoordsBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * texCoords.size(), texCoords.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint texCoordsLocation = glGetAttribLocation(grassShader, "texture");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(texCoordsLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS

    // Создаём буфер для позиций травинок
    glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint positionLocation = glGetAttribLocation(grassShader, "position");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS

    // создаем буфер для угла поворота
    glGenBuffers(1, &turningBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, turningBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassTurning.size(), grassTurning.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint turningLocation = glGetAttribLocation(grassShader, "turning");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(turningLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(turningLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(turningLocation, 1);                                  CHECK_GL_ERRORS

    // создаем буфер для масштаба
    glGenBuffers(1, &scaleBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * grassScale.size(), grassScale.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint scaleLocation = glGetAttribLocation(grassShader, "scale");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(scaleLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(scaleLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(scaleLocation, 1);                                  CHECK_GL_ERRORS

    // Создаём буфер для цвета 1
    GLuint green1Buffer;
    glGenBuffers(1, &green1Buffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, green1Buffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * colorGreen1.size(), colorGreen1.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint green1Location = glGetAttribLocation(grassShader, "green1");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(green1Location);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(green1Location, 3, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS

    // Создаём буфер для цвета 2
    GLuint green2Buffer;
    glGenBuffers(1, &green2Buffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, green2Buffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * colorGreen2.size(), colorGreen2.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint green2Location = glGetAttribLocation(grassShader, "green2");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(green2Location);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(green2Location, 3, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS

    // Создаём буфер для цвета 3
    GLuint green3Buffer;
    glGenBuffers(1, &green3Buffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, green3Buffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * colorGreen3.size(), colorGreen3.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint green3Location = glGetAttribLocation(grassShader, "green3");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(green3Location);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(green3Location, 3, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS

    // Создаём буфер для цвета 4
    GLuint yellowBuffer;
    glGenBuffers(1, &yellowBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, yellowBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * colorYellow.size(), colorYellow.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint yellowLocation = glGetAttribLocation(grassShader, "yellow");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(yellowLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(yellowLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS

    // создаем буфер для номеров цветов
    glGenBuffers(1, &colorBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(int) * grassColors.size(), grassColors.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint colorLocation = glGetAttribLocation(grassShader, "whichColor");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(colorLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(colorLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(colorLocation, 1);                                  CHECK_GL_ERRORS

    // Создаём буфер для смещения травинок
    glGenBuffers(1, &grassVariance);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint varianceLocation = glGetAttribLocation(grassShader, "variance");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(varianceLocation, 1);                                  CHECK_GL_ERRORS

    // Отвязываем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS

    // загружаем и создаём текстуру для травы
    glGenTextures(1, &grassTexture);                                            CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, grassTexture);                                 CHECK_GL_ERRORS

    // основние параметры отрисовки - по умолчанию GL_REPEAT

    // основные параметры интерполяции
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);            CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);            CHECK_GL_ERRORS

    // загрузка, создание текстуры
    int width, height;
    unsigned char* image = SOIL_load_image("../Texture/grass.jpg", &width, &height, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image); CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                             CHECK_GL_ERRORS

    SOIL_free_image_data(image);
    // убираем текстуру с редактирования
    glBindTexture(GL_TEXTURE_2D, 0);                                             CHECK_GL_ERRORS
}

// Создаём камеру (Если шаблонная камера вам не нравится, то можете переделать, но я бы не стал)
void CreateCamera() {
    camera.angle = 45.0f / 180.0f * M_PI;
    camera.direction = VM::vec3(0, 0.3, -1);
    camera.position = VM::vec3(0.5, 0.2, 0);
    camera.screenRatio = (float)screenWidth / screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

// Создаём замлю
void CreateGround() {
    // Земля состоит из двух треугольников
    vector<VM::vec4> meshPoints = {
        VM::vec4(0, 0, 0, 1),
        VM::vec4(1, 0, 0, 1),
        VM::vec4(1, 0, 1, 1),
        VM::vec4(0, 0, 0, 1),
        VM::vec4(1, 0, 1, 1),
        VM::vec4(0, 0, 1, 1),
    };

    // Подробнее о том, как это работает читайте в функции CreateGrass

    groundShader = GL::CompileShaderProgram("ground");

    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * meshPoints.size(), meshPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    glGenVertexArrays(1, &groundVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    // это координаты вершин
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);              CHECK_GL_ERRORS
    glEnableVertexAttribArray(0);                                                CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS


    // загружаем и создаём текстуру земли
    glGenTextures(1, &groundTexture);                                            CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, groundTexture);                                 CHECK_GL_ERRORS

    // основние параметры отрисовки - по умолчанию GL_REPEAT

    // основные параметры интерполяции
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);            CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);            CHECK_GL_ERRORS

    // загрузка, создание текстуры
    int width, height;
    unsigned char* image = SOIL_load_image("../Texture/my_ground_2.jpg", &width, &height, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image); CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                             CHECK_GL_ERRORS

    SOIL_free_image_data(image);
    // убираем текстуру с редактирования
    glBindTexture(GL_TEXTURE_2D, 0);                                             CHECK_GL_ERRORS
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    putenv("MESA_GL_VERSION_OVERRIDE=3.3COMPAT");
    try {
        cout << "Start" << endl;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
        glewInit();
        cout << "glew inited" << endl;
        CreateCamera();
        cout << "Camera created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}
