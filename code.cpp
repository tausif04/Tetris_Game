
#define _CRT_SECURE_NO_WARNINGS

// Suppress "function can be made static" informational warnings
#pragma warning(disable : 4505)  // unreferenced local function removed
#pragma warning(disable : 26812) // enum vs enum class

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

// ===================== CONSTANTS =====================
const int BOARD_COLS = 10;
const int BOARD_ROWS = 20;
const int CELL_SIZE = 32; // pixels per cell

// Layout
const int BOARD_X = 20; // board left edge
const int BOARD_Y = 60; // board top edge
const int PANEL_X = BOARD_X + BOARD_COLS * CELL_SIZE + 20;
const int PANEL_W = 180;
const int WIN_W = PANEL_X + PANEL_W + 20;
const int WIN_H = BOARD_Y + BOARD_ROWS * CELL_SIZE + 40;

// Fall speed (ms between auto-drops)
const int BASE_SPEED = 600;

// ===================== TETROMINOES =====================
// Each piece: 4 rotations x 4 cells (row,col offsets from pivot)
struct PieceDef
{
    int cells[4][4][2]; // [rotation][cell][row/col]
    float r, g, b;
};

// 7 standard tetrominoes
const PieceDef PIECES[7] = {
    // I - cyan
    {{{{0, 0}, {0, 1}, {0, 2}, {0, 3}},
      {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
      {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
      {{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
     0.0f,
     1.0f,
     1.0f},
    // O - yellow
    {{{{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}},
     1.0f,
     1.0f,
     0.0f},
    // T - purple
    {{{{0, 1}, {1, 0}, {1, 1}, {1, 2}},
      {{0, 0}, {1, 0}, {2, 0}, {1, 1}},
      {{1, 0}, {1, 1}, {1, 2}, {2, 1}},
      {{0, 1}, {1, 1}, {2, 1}, {1, 0}}},
     0.6f,
     0.0f,
     0.8f},
    // S - green
    {{{{0, 1}, {0, 2}, {1, 0}, {1, 1}},
      {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
      {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
      {{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
     0.0f,
     0.85f,
     0.2f},
    // Z - red
    {{{{0, 0}, {0, 1}, {1, 1}, {1, 2}},
      {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
      {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
      {{0, 1}, {1, 0}, {1, 1}, {2, 0}}},
     0.95f,
     0.1f,
     0.1f},
    // J - blue
    {{{{0, 0}, {1, 0}, {1, 1}, {1, 2}},
      {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
      {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
      {{0, 1}, {1, 1}, {2, 0}, {2, 1}}},
     0.1f,
     0.3f,
     1.0f},
    // L - orange
    {{{{0, 2}, {1, 0}, {1, 1}, {1, 2}},
      {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
      {{1, 0}, {1, 1}, {1, 2}, {2, 0}},
      {{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
     1.0f,
     0.55f,
     0.0f}};

// ===================== GAME STATE =====================
int board[BOARD_ROWS][BOARD_COLS]; // 0=empty, 1-7=color index
float boardColor[BOARD_ROWS][BOARD_COLS][3];

int curPiece, curRot, curRow, curCol;
int nextPiece;
int score = 0;
int level = 1;
int linesCleared = 0;
bool gameOver = false;
bool paused = false;

time_t startTime;
int elapsedSecs = 0;

// ===================== UTILITIES =====================
void setColor(float r, float g, float b) { glColor3f(r, g, b); }

// Draw a filled rectangle (pixel coords, Y-up from bottom)
void drawRect(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// ==================== DDA LINE ALGORITHM ====================
// Used for drawing the board grid lines
void ddaLine(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0)
        return;
    float xInc = (float)dx / steps;
    float yInc = (float)dy / steps;
    float x = (float)x1, y = (float)y1;
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++)
    {
        glVertex2f(roundf(x), roundf(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

// ==================== BRESENHAM LINE ALGORITHM ====================
// Used for drawing block outlines (borders of tetrominoes)
void bresenhamLine(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    glBegin(GL_POINTS);
    while (true)
    {
        glVertex2f((float)x1, (float)y1);
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
    glEnd();
}

// Draw a rectangle outline using Bresenham lines
void bresenhamRect(int x, int y, int w, int h)
{
    bresenhamLine(x, y, x + w, y);
    bresenhamLine(x + w, y, x + w, y + h);
    bresenhamLine(x + w, y + h, x, y + h);
    bresenhamLine(x, y + h, x, y);
}

// ===================== COORDINATE HELPERS =====================
// Convert board (row,col) to screen pixel (bottom-left of cell), Y-up
int screenX(int col) { return BOARD_X + col * CELL_SIZE; }
int screenY(int row) { return WIN_H - BOARD_Y - (row + 1) * CELL_SIZE; }

// ===================== TEXT RENDERING =====================
void drawText(float x, float y, const std::string &s, void *font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for (char c : s)
        glutBitmapCharacter(font, c);
}

void drawTextLarge(float x, float y, const std::string &s)
{
    drawText(x, y, s, GLUT_BITMAP_TIMES_ROMAN_24);
}

// ===================== BOARD FUNCTIONS =====================
void clearBoard()
{
    memset(board, 0, sizeof(board));
    memset(boardColor, 0, sizeof(boardColor));
}

bool isValidPos(int piece, int rot, int row, int col)
{
    for (int i = 0; i < 4; i++)
    {
        int r = row + PIECES[piece].cells[rot][i][0];
        int c = col + PIECES[piece].cells[rot][i][1];
        if (r < 0 || r >= BOARD_ROWS || c < 0 || c >= BOARD_COLS)
            return false;
        if (board[r][c])
            return false;
    }
    return true;
}

void placePiece()
{
    for (int i = 0; i < 4; i++)
    {
        int r = curRow + PIECES[curPiece].cells[curRot][i][0];
        int c = curCol + PIECES[curPiece].cells[curRot][i][1];
        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS)
        {
            board[r][c] = curPiece + 1;
            boardColor[r][c][0] = PIECES[curPiece].r;
            boardColor[r][c][1] = PIECES[curPiece].g;
            boardColor[r][c][2] = PIECES[curPiece].b;
        }
    }
}

int clearLines()
{
    int cleared = 0;
    for (int r = BOARD_ROWS - 1; r >= 0; r--)
    {
        bool full = true;
        for (int c = 0; c < BOARD_COLS; c++)
            if (!board[r][c])
            {
                full = false;
                break;
            }
        if (full)
        {
            cleared++;
            for (int rr = r; rr > 0; rr--)
            {
                for (int c = 0; c < BOARD_COLS; c++)
                {
                    board[rr][c] = board[rr - 1][c];
                    boardColor[rr][c][0] = boardColor[rr - 1][c][0];
                    boardColor[rr][c][1] = boardColor[rr - 1][c][1];
                    boardColor[rr][c][2] = boardColor[rr - 1][c][2];
                }
            }
            for (int c = 0; c < BOARD_COLS; c++)
            {
                board[0][c] = 0;
                boardColor[0][c][0] = boardColor[0][c][1] = boardColor[0][c][2] = 0;
            }
            r++; // re-check same row
        }
    }
    return cleared;
}

void spawnPiece()
{
    curPiece = nextPiece;
    nextPiece = rand() % 7;
    curRot = 0;
    curRow = 0;
    curCol = BOARD_COLS / 2 - 2;
    if (!isValidPos(curPiece, curRot, curRow, curCol))
    {
        gameOver = true;
    }
}

void newGame()
{
    clearBoard();
    score = 0;
    level = 1;
    linesCleared = 0;
    gameOver = false;
    paused = false;
    nextPiece = rand() % 7;
    startTime = time(NULL);
    elapsedSecs = 0;
    spawnPiece();
}

// ===================== DRAWING =====================

// Draw a single filled cell with 3D shading effect
void drawCell(int px, int py, float r, float g, float b)
{
    // Main fill
    setColor(r, g, b);
    drawRect(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2);

    // Light edge (top-left)
    setColor(fminf(r + 0.4f, 1.0f), fminf(g + 0.4f, 1.0f), fminf(b + 0.4f, 1.0f));
    drawRect(px + 1, py + CELL_SIZE - 3, CELL_SIZE - 2, 2); // top
    drawRect(px + 1, py + 1, 2, CELL_SIZE - 4);             // left

    // Dark edge (bottom-right)
    setColor(r * 0.45f, g * 0.45f, b * 0.45f);
    drawRect(px + 1, py + 1, CELL_SIZE - 2, 2);             // bottom
    drawRect(px + CELL_SIZE - 3, py + 1, 2, CELL_SIZE - 2); // right

    // Bresenham border outline
    setColor(0.1f, 0.1f, 0.1f);
    bresenhamRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1);
}

// Draw board background + DDA grid
void drawBoard()
{
    // Board background
    setColor(0.05f, 0.05f, 0.12f);
    drawRect(BOARD_X - 2, WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE - 2,
             BOARD_COLS * CELL_SIZE + 4, BOARD_ROWS * CELL_SIZE + 4);

    // DDA grid lines (subtle)
    setColor(0.15f, 0.15f, 0.3f);
    int bx1 = BOARD_X, bx2 = BOARD_X + BOARD_COLS * CELL_SIZE;
    int by1 = WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE;
    int by2 = WIN_H - BOARD_Y;

    // Vertical lines via DDA
    for (int c = 0; c <= BOARD_COLS; c++)
    {
        int x = BOARD_X + c * CELL_SIZE;
        ddaLine(x, by1, x, by2);
    }
    // Horizontal lines via DDA
    for (int r = 0; r <= BOARD_ROWS; r++)
    {
        int y = WIN_H - BOARD_Y - r * CELL_SIZE;
        ddaLine(bx1, y, bx2, y);
    }

    // Placed cells
    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
        {
            if (board[r][c])
            {
                drawCell(screenX(c), screenY(r),
                         boardColor[r][c][0], boardColor[r][c][1], boardColor[r][c][2]);
            }
        }
    }
}

// Draw ghost piece (where current piece would land)
void drawGhost()
{
    int ghostRow = curRow;
    while (isValidPos(curPiece, curRot, ghostRow + 1, curCol))
        ghostRow++;
    if (ghostRow == curRow)
        return;

    float gr = PIECES[curPiece].r * 0.25f;
    float gg = PIECES[curPiece].g * 0.25f;
    float gb = PIECES[curPiece].b * 0.25f;

    for (int i = 0; i < 4; i++)
    {
        int r = ghostRow + PIECES[curPiece].cells[curRot][i][0];
        int c = curCol + PIECES[curPiece].cells[curRot][i][1];
        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS)
        {
            int px = screenX(c), py = screenY(r);
            setColor(gr, gg, gb);
            drawRect(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2);
            // Bresenham outline on ghost
            setColor(PIECES[curPiece].r * 0.5f, PIECES[curPiece].g * 0.5f, PIECES[curPiece].b * 0.5f);
            bresenhamRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1);
        }
    }
}

// Draw current active piece
void drawCurrentPiece()
{
    for (int i = 0; i < 4; i++)
    {
        int r = curRow + PIECES[curPiece].cells[curRot][i][0];
        int c = curCol + PIECES[curPiece].cells[curRot][i][1];
        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS)
        {
            drawCell(screenX(c), screenY(r), PIECES[curPiece].r, PIECES[curPiece].g, PIECES[curPiece].b);
        }
    }
}

// Draw side panel
void drawPanel()
{
    int px = PANEL_X;
    int py = WIN_H - BOARD_Y;

    // Panel background
    setColor(0.08f, 0.08f, 0.18f);
    drawRect(px - 5, WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE - 2, PANEL_W, BOARD_ROWS * CELL_SIZE + 4);

    // --- NEXT PIECE ---
    int npBoxY = py - 30;
    setColor(0.9f, 0.9f, 0.9f);
    drawTextLarge(px, (float)npBoxY, "NEXT");

    int npCellSize = 28;
    int npBaseX = px + 10;
    int npBaseY = npBoxY - 20 - 4 * npCellSize;

    // Box for next piece using Bresenham border
    setColor(0.3f, 0.3f, 0.5f);
    bresenhamRect(npBaseX - 4, npBaseY - 4, 4 * npCellSize + 8, 4 * npCellSize + 8);

    setColor(0.05f, 0.05f, 0.12f);
    drawRect(npBaseX - 3, npBaseY - 3, 4 * npCellSize + 6, 4 * npCellSize + 6);

    // Draw next piece cells
    for (int i = 0; i < 4; i++)
    {
        int r = PIECES[nextPiece].cells[0][i][0];
        int c = PIECES[nextPiece].cells[0][i][1];
        int cx = npBaseX + c * npCellSize;
        int cy = npBaseY + (3 - r) * npCellSize;

        // Fill
        setColor(PIECES[nextPiece].r, PIECES[nextPiece].g, PIECES[nextPiece].b);
        drawRect(cx + 1, cy + 1, npCellSize - 2, npCellSize - 2);

        // Highlight
        setColor(fminf(PIECES[nextPiece].r + 0.4f, 1.0f),
                 fminf(PIECES[nextPiece].g + 0.4f, 1.0f),
                 fminf(PIECES[nextPiece].b + 0.4f, 1.0f));
        drawRect(cx + 1, cy + npCellSize - 3, npCellSize - 2, 2);
        drawRect(cx + 1, cy + 1, 2, npCellSize - 4);

        // Bresenham border
        setColor(0.1f, 0.1f, 0.1f);
        bresenhamRect(cx, cy, npCellSize - 1, npCellSize - 1);
    }

    int infoY = npBaseY - 20;

    // --- SCORE ---
    setColor(1.0f, 0.8f, 0.2f);
    drawTextLarge(px, (float)infoY, "SCORE");
    infoY -= 28;
    setColor(1.0f, 1.0f, 1.0f);
    drawTextLarge(px, (float)infoY, std::to_string(score));
    infoY -= 44;

    // --- LEVEL ---
    setColor(0.4f, 1.0f, 0.6f);
    drawTextLarge(px, (float)infoY, "LEVEL");
    infoY -= 28;
    setColor(1.0f, 1.0f, 1.0f);
    drawTextLarge(px, (float)infoY, std::to_string(level));
    infoY -= 44;

    // --- LINES ---
    setColor(0.4f, 0.8f, 1.0f);
    drawTextLarge(px, (float)infoY, "LINES");
    infoY -= 28;
    setColor(1.0f, 1.0f, 1.0f);
    drawTextLarge(px, (float)infoY, std::to_string(linesCleared));
    infoY -= 44;

    // --- TIME ---
    setColor(1.0f, 0.5f, 0.8f);
    drawTextLarge(px, (float)infoY, "TIME");
    infoY -= 28;
    int mm = elapsedSecs / 60, ss = elapsedSecs % 60;
    char timeBuf[16];
    sprintf_s(timeBuf, sizeof(timeBuf), "%02d:%02d", mm, ss);
    setColor(1.0f, 1.0f, 1.0f);
    drawTextLarge(px, (float)infoY, std::string(timeBuf));
    infoY -= 55;

    // --- CONTROLS ---
    setColor(0.6f, 0.6f, 0.6f);
    drawText(px, (float)infoY, "CONTROLS:", GLUT_BITMAP_HELVETICA_12);
    infoY -= 16;
    drawText(px, (float)infoY, "Left/Right: Move", GLUT_BITMAP_HELVETICA_12);
    infoY -= 14;
    drawText(px, (float)infoY, "Up: Rotate", GLUT_BITMAP_HELVETICA_12);
    infoY -= 14;
    drawText(px, (float)infoY, "Down: Soft Drop", GLUT_BITMAP_HELVETICA_12);
    infoY -= 14;
    drawText(px, (float)infoY, "Space: Hard Drop", GLUT_BITMAP_HELVETICA_12);
    infoY -= 14;
    drawText(px, (float)infoY, "P: Pause   R: Reset", GLUT_BITMAP_HELVETICA_12);
}

// Draw title
void drawTitle()
{
    setColor(0.3f, 0.7f, 1.0f);
    drawTextLarge(BOARD_X, WIN_H - 30, "TETRIS");
    setColor(0.5f, 0.5f, 0.5f);
    drawText(BOARD_X + 100, WIN_H - 33, "DDA + Bresenham Edition", GLUT_BITMAP_HELVETICA_12);
}

// Draw overlays
void drawOverlay(const std::string &line1, const std::string &line2)
{
    // Dim overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    drawRect(BOARD_X - 2, WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE - 2,
             BOARD_COLS * CELL_SIZE + 4, BOARD_ROWS * CELL_SIZE + 4);
    glDisable(GL_BLEND);

    setColor(1.0f, 0.3f, 0.3f);
    float tw = line1.size() * 14.0f;
    drawTextLarge(BOARD_X + (BOARD_COLS * CELL_SIZE - tw) / 2.0f,
                  WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE / 2.0f + 20,
                  line1);
    setColor(0.9f, 0.9f, 0.9f);
    float tw2 = line2.size() * 10.0f;
    drawText(BOARD_X + (BOARD_COLS * CELL_SIZE - tw2) / 2.0f,
             WIN_H - BOARD_Y - BOARD_ROWS * CELL_SIZE / 2.0f - 10,
             line2, GLUT_BITMAP_HELVETICA_18);
}

// ===================== GLUT CALLBACKS =====================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Window background
    setColor(0.03f, 0.03f, 0.08f);
    drawRect(0, 0, WIN_W, WIN_H);

    drawTitle();
    drawBoard();

    if (!gameOver && !paused)
    {
        drawGhost();
        drawCurrentPiece();
    }

    drawPanel();

    if (paused && !gameOver)
        drawOverlay("PAUSED", "Press P to continue");
    if (gameOver)
        drawOverlay("GAME OVER", "Press R to restart");

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Auto-drop timer
void timer(int value)
{
    if (!gameOver && !paused)
    {
        // Update elapsed time
        elapsedSecs = (int)difftime(time(NULL), startTime);

        // Try to move piece down
        if (isValidPos(curPiece, curRot, curRow + 1, curCol))
        {
            curRow++;
        }
        else
        {
            placePiece();
            int lines = clearLines();
            if (lines > 0)
            {
                linesCleared += lines;
                int pts[] = {0, 100, 300, 500, 800};
                score += pts[lines] * level;
                level = 1 + linesCleared / 10;
            }
            spawnPiece();
        }
    }
    glutPostRedisplay();
    int speed = BASE_SPEED - (level - 1) * 50;
    if (speed < 80)
        speed = 80;
    glutTimerFunc(speed, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'r' || key == 'R')
    {
        newGame();
        glutPostRedisplay();
        return;
    }
    if (key == 'p' || key == 'P')
    {
        paused = !paused;
        if (!paused)
            startTime = time(NULL) - elapsedSecs;
        glutPostRedisplay();
        return;
    }
    if (gameOver || paused)
        return;

    if (key == ' ')
    { // Hard drop
        while (isValidPos(curPiece, curRot, curRow + 1, curCol))
            curRow++;
        placePiece();
        int lines = clearLines();
        if (lines > 0)
        {
            linesCleared += lines;
            int pts[] = {0, 100, 300, 500, 800};
            score += pts[lines] * level;
            level = 1 + linesCleared / 10;
        }
        spawnPiece();
        glutPostRedisplay();
    }
}

void specialKeys(int key, int x, int y)
{
    if (gameOver || paused)
        return;
    switch (key)
    {
    case GLUT_KEY_LEFT:
        if (isValidPos(curPiece, curRot, curRow, curCol - 1))
            curCol--;
        break;
    case GLUT_KEY_RIGHT:
        if (isValidPos(curPiece, curRot, curRow, curCol + 1))
            curCol++;
        break;
    case GLUT_KEY_DOWN:
        if (isValidPos(curPiece, curRot, curRow + 1, curCol))
            curRow++;
        break;
    case GLUT_KEY_UP:
    {
        int newRot = (curRot + 1) % 4;
        if (isValidPos(curPiece, newRot, curRow, curCol))
            curRot = newRot;
        else if (isValidPos(curPiece, newRot, curRow, curCol + 1))
        {
            curCol++;
            curRot = newRot;
        }
        else if (isValidPos(curPiece, newRot, curRow, curCol - 1))
        {
            curCol--;
            curRot = newRot;
        }
        break;
    }
    }
    glutPostRedisplay();
}

// ===================== MAIN =====================
int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("TETRIS - DDA & Bresenham Edition");

    glClearColor(0.03f, 0.03f, 0.08f, 1.0f);
    glPointSize(1.0f);

    newGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(BASE_SPEED, timer, 0);

    glutMainLoop();
    return 0;
}