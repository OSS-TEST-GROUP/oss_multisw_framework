#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <ncursesw/curses.h>   
#include <ncursesw/panel.h>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <cstring>
#include <cstdarg>
#include <functional>
#include <algorithm>
#include <sstream>
#include <queue>
#include <unistd.h>
#include <chrono>
#include <ncursesw/form.h>
#include <clocale>
#include <iomanip>
#include <atomic>

#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <mutex>

enum class LogPriority {
      UNKNOWN = 0,   // ANDROID_LOG_UNKNOWN = 0,
      DEFAULT,       // ANDROID_LOG_DEFAULT,
      VERBOSE,       // ANDROID_LOG_VERBOSE,
      DEBUG,         // ANDROID_LOG_DEBUG,
      INFO,          // ANDROID_LOG_INFO,
      WARN,          // ANDROID_LOG_WARN,
      ERROR,         // ANDROID_LOG_ERROR,
      FATAL,         // ANDROID_LOG_FATAL,
      SILENT         // ANDROID_LOG_SILENT
};

#if 1
// A simple thread-safe file logger
class FileLogger {
public:
    static FileLogger& getInstance() {
        static FileLogger instance;
        return instance;
    }

    using LogCallback = std::function<void(const std::string&)>;
    void setCallback(LogCallback cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = cb;
    }

    void write(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        
        std::string full_msg = ss.str() + message;
        log_file_ << full_msg << std::endl;
        if (callback_) callback_(full_msg);
    }

private:
    FileLogger() {
        // Open the log file in append mode.
        // The file will be created in the directory where the executable is run.
        log_file_.open("test_log.txt", std::ios::out | std::ios::app);
    }

    ~FileLogger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    // Disable copy and move
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    std::ofstream log_file_;
    std::mutex mutex_;
    LogCallback callback_;
};
#endif

class LogStream : public std::ostringstream {
public:
    LogStream(LogPriority priority) : priority_(priority) {}

    ~LogStream() {
        // Write to both cout/cerr and the file for tests
        //std::ostream& stream = (priority_ == LogPriority::INFO) ? std::cout : std::cerr;
        //stream << str() << std::endl;
        FileLogger::getInstance().write(str());

    }

private:
    LogPriority priority_;
};

#define LOG_INFO LogStream(LogPriority::INFO)
#define LOG_ERROR LogStream(LogPriority::ERROR)
#define LOG_WARN LogStream(LogPriority::WARN)
#define LOG_DEBUG LogStream(LogPriority::DEBUG)
#endif // LOGGER_H


namespace nui {

enum Orientation { HORIZONTAL = 0, VERTICAL = 1 };

struct Event {
    enum Type { KEY, MOUSE } type;
    int code;   // Key code or Mouse button info
    int x, y;
    bool pressed;
};

class InputManager {
public:
    enum Mode { NCURSES_MODE, RAW_MODE };

    InputManager() : m_mode(NCURSES_MODE), m_lastClickTime(0), m_clickCount(0), 
                     m_lastX(-1), m_lastY(-1), m_lastBtn(-1) {}
    ~InputManager() { finalize(); }

    void setMode(Mode mode) { 
        m_mode = mode; 
        initialize();
    }

    void initialize() {
        //keypad(stdscr, TRUE); // 이스케이프 시퀀스 조립을 위해 항상 TRUE 유지
        if (m_mode == RAW_MODE) {
            // Disable ncurses keypad and mouse handling to get raw sequences
            keypad(stdscr, FALSE);
            mousemask(0, NULL);
            // Enable all mouse motion (1003) and SGR extended mode (1006)
            printf("\033[?1003h\033[?1006h"); 
            fflush(stdout);
        } else {
            keypad(stdscr, TRUE);
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        }
    }

    void finalize() {
        if (m_mode == RAW_MODE) {
            // Disable mouse reporting
            printf("\033[?1003l\033[?1006l"); 
            fflush(stdout);
        }
    }

    Mode getMode() const { return m_mode; }

    void poll() {
        int ch = getch();
        //LOG_INFO << "Received key: " << ch;

        if (ch == ERR) return;
        
                // 헬퍼 람다 함수 (내부에서 키 이벤트를 밀어넣음)
        auto pushKeyEvent = [this](int code) {
            Event ev;
            ev.type = Event::KEY;
            ev.code = code;
            m_queue.push(ev);
        };

        if (m_mode == NCURSES_MODE) {
            if (ch == KEY_MOUSE) {
                MEVENT ev;
                if (getmouse(&ev) == OK) {
                    Event event;
                    event.type = Event::MOUSE;
                    event.code = ev.bstate;
                    event.x = ev.x;
                    event.y = ev.y;
                    event.pressed = (ev.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED));
                    m_queue.push(event);
                }
            } else {
                Event event;
                event.type = Event::KEY;
                event.code = ch;
                m_queue.push(event);
            }
        } else {
            if (ch == 27) { // ESC
                std::string seq;
                seq += (char)ch;
                
                int retry = 0;
                while (retry < 200) { 
                    int next = getch();
                    if (next == ERR) { retry++; usleep(500); continue; }
                    seq += (char)next;
                    if (next == 'M' || next == 'm') break; 
                    if (seq.size() > 30) break; 
                    retry = 0;
                }

                if (seq.size() > 3 && seq[1] == '[' && seq[2] == '<') {
                    parseSGR(seq);
                } else {
                    // 특수키 매핑
                    if (seq == "\033[A") pushKeyEvent(KEY_UP);
                    else if (seq == "\033[B") pushKeyEvent(KEY_DOWN);
                    else if (seq == "\033[C") pushKeyEvent(KEY_RIGHT);
                    else if (seq == "\033[D") pushKeyEvent(KEY_LEFT);
                    else if (seq == "\033[H") pushKeyEvent(KEY_HOME);
                    else if (seq == "\033[F") pushKeyEvent(KEY_END);
                    else if (seq == "\033[3~") pushKeyEvent(KEY_DC); // Delete
                    else {
                        // 해석 안 된 시퀀스는 개별 문자로 전송
                        for (char c : seq) pushKeyEvent((unsigned char)c);
                    }
                }
            } else if (ch == KEY_MOUSE) {
                MEVENT ev;
                if (getmouse(&ev) == OK) {
                    Event event; event.type = Event::MOUSE; event.code = ev.bstate;
                    event.x = ev.x; event.y = ev.y; event.pressed = true;
                    m_queue.push(event);
                }
            } else {
                pushKeyEvent(ch);
            }
        }
    }

    void poll_v2() {
        int ch = getch();
        if (ch == ERR) return;
        
        auto pushKeyEvent = [this](int code) {
            Event ev; ev.type = Event::KEY; ev.code = code;
            m_queue.push(ev);
        };

        if (ch == KEY_MOUSE) {
            MEVENT ev;
            if (getmouse(&ev) == OK) {
                Event event; event.type = Event::MOUSE; event.code = ev.bstate;
                event.x = ev.x; event.y = ev.y;
                event.pressed = (ev.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED));
                m_queue.push(event);
            }
            return;
        }

        if (m_mode == RAW_MODE && ch == 27) {
            std::string seq;
            seq += (char)ch;
            int next = getch();
            if (next == '[') {
                seq += (char)next;
                next = getch();
                if (next == '<') {
                    seq += (char)next;
                    int retry = 0;
                    while (retry < 20) {
                        next = getch();
                        if (next == ERR) { retry++; usleep(1000); continue; }
                        seq += (char)next;
                        if (next == 'm' || next == 'M') { parseSGR(seq); return; }
                        retry = 0;
                    }
                } else if (next != ERR) ungetch(next);
            } else if (next != ERR) ungetch(next);
            pushKeyEvent(27);
        } else {
            pushKeyEvent(ch);
        }
    }

    bool popEvent(Event& ev) {
        if (m_queue.empty()) return false;
        ev = m_queue.front();
        m_queue.pop();
        return true;
    }

private:
    void parseSGR(const std::string& seq) {
        try {
            size_t start = 3; 
            size_t end = seq.find(';', start);
            if (end == std::string::npos) return;
            int pb = std::stoi(seq.substr(start, end - start));

            start = end + 1;
            end = seq.find(';', start);
            if (end == std::string::npos) return;
            int px = std::stoi(seq.substr(start, end - start));

            start = end + 1;
            char last = seq.back();
            int py = std::stoi(seq.substr(start, seq.size() - 1 - start));

            Event ev;
            ev.type = Event::MOUSE;
            ev.x = px - 1;
            ev.y = py - 1;
            ev.pressed = (last == 'M');
            ev.code = 0;

            bool isMotion = (pb & 32);
            bool isWheel = (pb & 64);

            if (isWheel) {
                ev.code = (pb == 64) ? BUTTON4_PRESSED : BUTTON5_PRESSED;
            } else if (isMotion) {
                ev.code = REPORT_MOUSE_POSITION;
                int btn = pb & 3;
                if (btn == 0) ev.code |= BUTTON1_PRESSED;
                else if (btn == 1) ev.code |= BUTTON2_PRESSED;
                else if (btn == 2) ev.code |= BUTTON3_PRESSED;
                // btn == 3 은 버튼 없이 이동하는 경우이므로 추가 플래그 없음
            } else {
                int btn = pb & 3;
                unsigned long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                if (last == 'M') { // Press
                    if (btn == 0) ev.code = BUTTON1_PRESSED;
                    else if (btn == 1) ev.code = BUTTON2_PRESSED;
                    else if (btn == 2) ev.code = BUTTON3_PRESSED;
                    m_pressTime = now;
                } else { // Release ('m')
                    unsigned int base = 0;
                    if (btn == 0) base = BUTTON1_RELEASED;
                    else if (btn == 1) base = BUTTON2_RELEASED;
                    else if (btn == 2) base = BUTTON3_RELEASED;
                    ev.code = base;

                    // Click detection (Interval 166ms)
                    LOG_DEBUG << "now - m_pressTime: " << now - m_pressTime;
                    LOG_DEBUG << "now - m_lastClickTime: " << now - m_lastClickTime;
                    LOG_DEBUG << "m_clickCount: " << m_clickCount;
                    LOG_DEBUG << "m_clickCount: " << m_clickCount << " m_lastX: " << m_lastX << " m_lastY: " << m_lastY << " m_lastBtn: " << m_lastBtn;
                    if (now - m_pressTime < 300) { // 단순 클릭 기간 체크
                        if (btn == m_lastBtn && ev.x == m_lastX && ev.y == m_lastY && (now - m_lastClickTime < 500)) {
                            m_clickCount++;
                        } else {
                            m_clickCount = 1;
                        }

                        if (m_clickCount == 1) ev.code |= (btn == 0 ? BUTTON1_CLICKED : (btn == 1 ? BUTTON2_CLICKED : BUTTON3_CLICKED));
                        else if (m_clickCount == 2) ev.code |= (btn == 0 ? BUTTON1_DOUBLE_CLICKED : (btn == 1 ? BUTTON2_DOUBLE_CLICKED : BUTTON3_DOUBLE_CLICKED));
                        else if (m_clickCount >= 3) ev.code |= (btn == 0 ? BUTTON1_TRIPLE_CLICKED : (btn == 1 ? BUTTON2_TRIPLE_CLICKED : BUTTON3_TRIPLE_CLICKED));

                        m_lastClickTime = now;
                        m_lastX = ev.x; m_lastY = ev.y; m_lastBtn = btn;
                    }
                }
            }
            m_queue.push(ev);
        } catch (...) {}
    }

    Mode m_mode;
    std::queue<Event> m_queue;
    unsigned long m_pressTime;
    unsigned long m_lastClickTime;
    int m_clickCount;
    int m_lastX, m_lastY, m_lastBtn;
};

#define WRAP_CONTENT -1
#define MATCH_PARENT -2
//#define WEIGHT_PARENT -3
#define NUI_COLOR(f, b) COLOR_PAIR((f) * 8 + (b) + 1)
/* ncurses 기본 색상 번호:
 * 0: BLACK, 1: RED, 2: GREEN, 3: YELLOW, 
 * 4: BLUE, 5: MAGENTA, 6: CYAN, 7: WHITE 
 */


struct Rect { int y, x, w, h; };

class View : public std::enable_shared_from_this<View> {
public:
    View() : m_win(nullptr), m_subWin(nullptr), m_panel(nullptr), m_originalRect{0,0,WRAP_CONTENT,WRAP_CONTENT}, m_rect{0,0,0,0}, m_visible(true), 
    m_focusable(false), m_isFocused(false), m_title(""), m_border(false), m_padding_left(0), m_isTouched(true),
    m_padding_right(0), m_padding_top(0), m_padding_bottom(0), m_clickable(false), m_isPressed(false), 
    m_weight(0), m_calW(0), m_calH(0), m_size_w(0), m_size_h(0), m_min_w(0), m_min_h(0), m_max_w(0), m_max_h(0),
    m_onClickListener(nullptr), m_name(""), m_editable(false), m_isEditing(false), m_prevCursorState(-1), m_showCursor(false) {
        m_win = newwin(1, 1, 0, 0);
        m_panel = new_panel(m_win);
    }

    void setParent(std::weak_ptr<View> parent) { m_parent = parent; }
    void setPopupHandler(std::function<void(std::shared_ptr<View>)> handler) { m_popupHandler = handler; }

    virtual void requestPopup(std::shared_ptr<View> popup) {
        auto p = m_parent.lock();
        if (p) p->requestPopup(popup);
        else if (m_popupHandler) m_popupHandler(popup);
    }
    virtual void requestClosePopup() {
        auto p = m_parent.lock();
        if (p) p->requestClosePopup();
        else if (m_popupHandler) m_popupHandler(nullptr);
    }
    
    virtual ~View() {
        if (m_panel) { del_panel(m_panel); m_panel = nullptr; }
        if (m_subWin) { delwin(m_subWin); m_subWin = nullptr; }
        if (m_win) { delwin(m_win); m_win = nullptr; }
    }

    virtual void onDraw(View* parentView) = 0;
    virtual void drawBackground(bool erase = true) {
        if (!m_win) return;

        chtype attr = A_NORMAL;

        if(m_isEditing) {
            LOG_DEBUG << "onDraw: m_showCursor=" << m_showCursor;
        }
        
        if(m_isPressed && !m_isEditing) {
            attr = NUI_COLOR(COLOR_YELLOW, COLOR_RED) | A_BOLD;
        } else if(m_isEditing && m_showCursor){
            erase = false;
        } else if(m_isEditing) {
            attr = NUI_COLOR(COLOR_BLACK, COLOR_MAGENTA) | A_BOLD;
        } else if(m_isFocused) {
            attr = NUI_COLOR(COLOR_BLACK, COLOR_YELLOW) | A_BOLD;
        } else {
            attr = A_NORMAL;
        }
        
        if (erase) { wbkgd(m_win, attr); werase(m_win); }
        else LOG_DEBUG << "erase false";

        if (m_border) box(m_win, 0, 0);
        if (!m_title.empty()) mvwprintw(m_win, 0, 1, "%s", m_title.c_str());
    }


    virtual bool dispatchMouseEvent(MEVENT* ev, WINDOW* parentWin) {
        if (!m_visible || !parentWin) return false;
        if (wenclose(m_win, ev->y, ev->x)) { 
            //LOG_INFO << "Mouse in view " << m_name << " y: " << ev->y << " x: " << ev->x << " w: " << m_rect.w << " h: " << m_rect.h;
            return onMouseEvent(ev); 
        }
        else { 
            //LOG_INFO << "Mouse not in view " << m_name << " y: " << ev->y << " x: " << ev->x << " w: " << m_rect.w << " h: " << m_rect.h;
            if(m_focusable) setFocused(false); 
            return false; 
        }
    }

    virtual bool dispatchKeyEvent(int ch) {
        if (!m_visible) return false;
        if (m_editable) {
            if (!m_isEditing) {
                setFocused(false);
                return false;
            }
        } else {
            if (!m_isFocused) return false;
        }
        return onKeyEvent(ch);
    }

    virtual bool onMouseEvent(MEVENT* ev) {
        m_isTouched = true;
        if(m_focusable && (ev->bstate & REPORT_MOUSE_POSITION)) {
            setFocused(true);
        }
        if(m_clickable){
            if(ev->bstate & BUTTON1_PRESSED) {
                m_isPressed = true;
            }
            if(ev->bstate & BUTTON1_RELEASED) {
                m_isPressed = false;
            }
            if(ev->bstate & BUTTON1_CLICKED) {
                if (m_editable) {
                    //m_isEditing = true;
                    setEditing(true);
                    top_panel(m_panel);
                }
                if (m_onClickListener) { m_onClickListener(); }
                return true;
            }
        }

        return false;
    }

    virtual bool onKeyEvent(int ch) { return false; }

    virtual void setRect(int y, int x, int w, int h) { 
        int my, mx; getmaxyx(stdscr, my, mx);

        if(w < m_min_w) w = m_min_w;
        if(h < m_min_h) h = m_min_h;

        if(y >= my) y = my-h; if(x >= mx) x = mx-w;
        if(y+h > my) h = my - y; if(x+w > mx) w = mx - x;
        m_rect = {y, x, w, h}; 
        if (!m_win) {
            m_win = newwin(h, w, y, x);
        if (m_win) {
                m_panel = new_panel(m_win);
                if (!m_panel) LOG_ERROR << "setRect: Failed to create panel for " << m_name;
            } else {
                LOG_ERROR << "setRect: Failed to create newwin for " << m_name;
            }
        } else {
            if(wresize(m_win, h, w) != OK) {
                LOG_ERROR << "setRect: wresize failed for " << m_name << " to " << w << "x" << h;
            }
            
            if(m_subWin) {
                wresize(m_subWin, h-m_padding_top-m_padding_bottom, w-m_padding_left-m_padding_right);
            }
        
            if (move_panel(m_panel, y, x) != OK) {
                LOG_ERROR << "setRect: move_panel failed for " << m_name << " to " << y << "," << x;
            }
        }
    }
    virtual void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) { }

    Rect getRect() const { return m_rect; }
    int getWidth() const { return m_rect.w; }
    int getHeight() const { return m_rect.h; }
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void setFocusable(bool f) { m_focusable = f; }
    bool isFocusable() const { return m_focusable; }
    virtual void setFocused(bool f) {
        m_isTouched = true;
        if(f != m_isFocused) {
            m_isFocused = f; 
            m_isTouched = true;
        }
    }
    bool isFocused() const { return m_isFocused; }
    void setOnClickListener(std::function<void()> listener) { m_onClickListener = listener; }
    void setTitle(std::string title) { m_title = title; }
    void setBorder(bool border) { m_border = border; }
    void setPadding(int l, int r, int t, int b) { 
        m_padding_left = l; m_padding_right = r; m_padding_top = t; m_padding_bottom = b;
        m_min_h = m_padding_top + m_padding_bottom + 1;
        m_min_w = m_padding_left + m_padding_right + 1;
        calculateDimensions();
    }
    void setWeight(int weight) { m_weight = weight; }
    int getWeight() const { return m_weight; }
    void setOriginalRect(int y, int x, int w, int h) { m_originalRect = {y, x, w, h}; }
    Rect getOriginalRect() const { return m_originalRect; }
    void setSize(int w, int h) { m_originalRect.w = w; m_originalRect.h = h; }
    void setMinSize(int w, int h) { m_min_w = w; m_min_h = h; }
    void setMaxSize(int w, int h) { m_max_w = w; m_max_h = h; }
    void setName(std::string name) { m_name = name; }
    void setClickable(bool clickable) { m_clickable = clickable; }
    bool isClickable() const { return m_clickable; }
    std::string getName() const { return m_name; }
    PANEL* getPanel() const { return m_panel; }
    virtual int getCalculatedWidth(int widthPerWeight, int maxWidth) const { 
        int ret = 0;
        if(m_originalRect.w == WRAP_CONTENT) {
            ret = m_calW+widthPerWeight*m_weight;     
        } else if(m_originalRect.w == MATCH_PARENT) {
            ret = maxWidth;
        } else {
            if(m_originalRect.w == 0 && widthPerWeight == 0) return 0;
            ret = m_originalRect.w+widthPerWeight*m_weight;
        }

        if(ret > maxWidth) ret = maxWidth;
        if(m_max_w > 0 && ret > m_max_w) ret = m_max_w;
        if(ret < std::min(m_min_w, m_calW)) ret = std::min(m_min_w, m_calW);
        return ret;
    }
    virtual int getRealWidth(int widthPerWeight, int maxWidth) const { 
        int ret = 0;
        // if(m_originalRect.w == WRAP_CONTENT || m_originalRect.w == 0) {
        //     ret = m_calW+widthPerWeight*m_weight;     
        // } else if(m_originalRect.w == MATCH_PARENT) {
        //     ret = maxWidth;
        // } else {
        //     ret = m_originalRect.w+widthPerWeight*m_weight;
        // }

        if(m_originalRect.w > 0 ) ret = m_originalRect.w;
        else ret = m_calW;
        
        if(ret > maxWidth) ret = maxWidth;
        if(m_max_w > 0 && ret > m_max_w) ret = m_max_w;
        if(ret < std::min(m_min_w, m_calW)) ret = std::min(m_min_w, m_calW);
        return ret;
    }

    virtual int getRealHeight(int heightPerWeight, int maxHeight) const { 
        int ret = 0;
        // if(m_originalRect.h == WRAP_CONTENT || m_originalRect.h == 0) {
        //     ret = m_calH+heightPerWeight*m_weight;     
        // } else if(m_originalRect.h == MATCH_PARENT) {
        //     ret = maxHeight;
        // } else {
        //     ret = m_originalRect.h+heightPerWeight*m_weight;
        // }

        if(m_originalRect.h > 0 ) ret = m_originalRect.h;
        else ret = m_calH;

        if(ret > maxHeight) ret = maxHeight;
        if(m_max_h > 0 && ret > m_max_h) ret = m_max_h;
        if(ret < m_min_h) ret = m_min_h;
        return ret;
    }
    virtual int getCalculatedHeight(int heightPerWeight, int maxHeight) const { 
        int ret = 0;
        if(m_originalRect.h == WRAP_CONTENT) {
            ret = m_calH+heightPerWeight*m_weight;     
        } else if(m_originalRect.h == MATCH_PARENT) {
            ret = maxHeight;
        } else {
            if(m_originalRect.h == 0 && heightPerWeight == 0) return 0;
            ret = m_originalRect.h+heightPerWeight*m_weight;
        }

        if(ret > maxHeight) ret = maxHeight;
        if(m_max_h > 0 && ret > m_max_h) ret = m_max_h;
        if(ret < m_min_h) ret = m_min_h;
        return ret;
    }

    void setEditable(bool e) { m_editable = e; }
    bool isEditable() const { return m_editable; }
    virtual void setEditing(bool e) { m_isEditing = e; showCursor(e); m_isTouched = true;}
    bool isEditing() const { return m_isEditing; }
    void showCursor(bool show) { 
        if(m_showCursor != show) {
            m_showCursor = show; 
            if(m_showCursor) curs_set(1);
            else curs_set(0);
        }
    }
    bool getIsTouched() const { return m_isTouched; }
    void setIsTouched(bool e) { m_isTouched = e; }

protected:
    WINDOW* m_win;
    WINDOW* m_subWin;
    PANEL* m_panel;
    Rect m_rect;
    Rect m_originalRect;
    bool m_visible, m_focusable, m_isFocused;
    bool m_editable, m_isEditing;
    bool m_clickable;
    bool m_isPressed;
    std::weak_ptr<View> m_parent;
    std::function<void(std::shared_ptr<View>)> m_popupHandler;
    std::function<void()> m_onClickListener;
    std::string m_title;
    bool m_border;
    int m_padding_left, m_padding_right, m_padding_top, m_padding_bottom;
    int m_weight;
    int m_calW, m_calH; // 계산된 권장 치수
    int m_size_w, m_size_h;
    int m_min_w, m_min_h;
    int m_max_w, m_max_h;
    std::string m_name;
    int m_prevCursorState; // 0: off, 1: on
    bool m_showCursor;
    bool m_isTouched;
};

class ViewGroup : public View {
public:
    ViewGroup(std::string un) : totalWeight(0) { m_name = un; }
    virtual ~ViewGroup() = default;

    void addView(std::shared_ptr<View> view) { 
        m_children.push_back(view);
        view->setParent(shared_from_this());
        if (view->isVisible()) totalWeight += view->getWeight();
        //if (view->getPanel()) top_panel(view->getPanel());
    }
    void removeAllViews() { m_children.clear(); totalWeight = 0; }

    void onDraw(View* parentView) override {
        if (!m_win || !isVisible()) return;
        drawBackground();
        for (auto& child : m_children) if (child->isVisible() /*&& child->getIsTouched()*/) {
            //LOG_INFO << "child onDraw" << child->m_name;
            child->onDraw(this); child->setIsTouched(false);
        }
    }

    // void drawBackground(bool erase = true) override {
    //     View::drawBackground(erase);
    // }

    virtual void requestLayout(int sY, int sX, int mW, int mH) {}

    virtual void updateLayout(int sY, int sX, int mW, int mH) {
        calculateDimensions(mW, mH);
        requestLayout(sY, sX, mW, mH);
    }

    bool dispatchMouseEvent(MEVENT* ev, WINDOW* parentWin) override {
        if (!m_visible || !m_win) return false;
        if (wenclose(m_win, ev->y, ev->x)) {
            bool ret = false;
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                //LOG_INFO << "ViewGroup: dispatchMouseEvent child: " << m_name << ", " << (*it)->getName() << " evt->y: " << ev->y << " evt->x: " << ev->x;
                if ((*it)->dispatchMouseEvent(ev, m_win)) { ret = true; }
                else if(ev->bstate & BUTTON1_CLICKED) {
                    (*it)->setEditing(false);
                }
            }
            if (ret) return true;
            return onMouseEvent(ev);
        }else{
            if(ev->bstate & BUTTON1_CLICKED) {
                setEditing(false);
            }
            setFocused(false);
            return false;
        }
    }

    void setEditing(bool e) override { 
        View::setEditing(e);
        for(size_t i=0; i<m_children.size(); i++) {
            m_children[i]->setEditing(e); 
        }
     }
    
    void setFocused(bool f) override {
        View::setFocused(f);
        for(size_t i=0; i<m_children.size(); i++) {
            m_children[i]->setFocused(f); 
            //auto group = std::dynamic_pointer_cast<ViewGroup>(m_children[i]);
            //if (group) group->resetFocusChild(false);
        }
    }

    bool dispatchKeyEvent(int ch) override {
        bool ret = false;
        for (auto& child : m_children) {
            LOG_INFO << "ViewGroup: dispatchKeyEvent child: " << m_name << ", " << child->getName() << " ch: " << ch;
            if (child->dispatchKeyEvent(ch)) { ret = true; }
        }
        if (ret) return ret;
        return View::dispatchKeyEvent(ch);
    }

    void requestFocusNext() {
        int current = -1;
        for(size_t i=0; i<m_children.size(); i++) {
            if (m_children[i]->isFocused()) { current = (int)i; m_children[i]->setFocused(false); break; }
        }
        for(size_t i=1; i<=m_children.size(); i++) {
            int next = (current + (int)i) % m_children.size();
            if (m_children[next]->isFocusable()) { m_children[next]->setFocused(true); break; }
        }
    }

    std::shared_ptr<View> findByName(std::string targetName) {
        if (m_name == targetName) return shared_from_this();
        for (auto& child : m_children) {
            if (child->getName() == targetName) return child;
            auto group = std::dynamic_pointer_cast<ViewGroup>(child);
            if (group) {
                auto found = group->findByName(targetName);
                if (found) return found;
            }
        }
        return nullptr;
    }

protected:
    std::vector<std::shared_ptr<View>> m_children;
    int totalWeight;
};

class LinearLayout : public ViewGroup {
public:
    LinearLayout(std::string un, Orientation o) 
        : ViewGroup(un), m_orientation(o) {}

    void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) override {
        int totalW = 0;
        int totalH = 0;
LOG_INFO << "calculateDimensions start: " << m_name << " : maxWidth: " << maxWidth << ", maxHeight: " << maxHeight;
        int availW = maxWidth - m_padding_left - m_padding_right;
        int availH = maxHeight - m_padding_top - m_padding_bottom;

        for (auto& child : m_children) {
            if (!child->isVisible()) continue;

            child->calculateDimensions(availW, availH);

            int cw = child->getRealWidth(0, availW);
            int ch = child->getRealHeight(0, availH);
            LOG_INFO << "LinearLayout: calculateDimensions " << m_name << " child: " << child->getName() << " cw: " << cw << " ch: " << ch;
            if (m_orientation == HORIZONTAL) {
                totalW += cw;
                totalH = std::max(totalH, ch);
                availW -= cw;
                if (availW < 0) availW = 0;
            } else {
                totalH += ch;
                totalW = std::max(totalW, cw);
                availH -= ch;
                if (availH < 0) availH = 0;
            }
        }
        
        m_calW = totalW + m_padding_left + m_padding_right; 
        m_calH = totalH + m_padding_top + m_padding_bottom;
        LOG_DEBUG << "calculateDimensions on " << m_name << " : " << m_calW << ", " << m_calH << " weight: " << getWeight();
    }
        
    void requestLayout(int sY, int sX, int mW, int mH) override {
        LOG_DEBUG << "requestLayout on " << m_name << " at (" << sY << ", " << sX << ") with size (" << mW << ", " << mH << ")";
        // 1. 가중치 합계 및 사용 가능한 내부 공간 계산
        int totalWeight = 0;
        int last_index = -1;
        for (size_t i = 0; i < m_children.size(); ++i) {
            auto& c = m_children[i];
            if (c->isVisible()) {
                int weight = c->getWeight();
                if (weight > 0) {
                    last_index = (int)i;
                }
                totalWeight += weight;
            }
        }

        int innerW = mW - m_padding_left - m_padding_right;
        int innerH = mH - m_padding_top - m_padding_bottom;
        
        // 2. Pass 1: 가중치 제외 순수 점유 공간 계산
        int totalNaturalSize = 0;
        int availSpacePass1 = (m_orientation == HORIZONTAL ? innerW : innerH);
        
        for (auto& c : m_children) {
            if (!c->isVisible()) continue;
            
            if (m_orientation == HORIZONTAL) {
                int cw = c->getCalculatedWidth(0, availSpacePass1);
                totalNaturalSize += cw;
                availSpacePass1 -= cw;
                if (availSpacePass1 < 0) availSpacePass1 = 0;
            } else {
                int ch = c->getCalculatedHeight(0, availSpacePass1);
                totalNaturalSize += ch;
                availSpacePass1 -= ch;
                if (availSpacePass1 < 0) availSpacePass1 = 0;
            }
        }

        LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << " totalNaturalSize: " << totalNaturalSize;
        // 3. 여유 공간 분배를 위한 단위 공간 계산
        int orientationSpace = (m_orientation == HORIZONTAL ? innerW : innerH);
        LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << " orientationSpace: " << orientationSpace;
        int remainingSpace = orientationSpace - totalNaturalSize;
        LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << " remainingSpace: " << remainingSpace;
        LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << " totalWeight: " << totalWeight;
        int unitSpace = 0;
        if (remainingSpace > 0 && totalWeight > 0) {
            unitSpace = remainingSpace / totalWeight;
        }
        LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << " unitSpace: " << unitSpace;

        // 4. Pass 2: 실제 좌표 할당 및 자식 배치
        int curY = sY + m_padding_top;
        int curX = sX + m_padding_left;
        int contentW = 0, contentH = 0;
        
        int layoutAvailW = innerW;
        int layoutAvailH = innerH;

        for (size_t i = 0; i < m_children.size(); ++i) {
            auto& c = m_children[i];
            
            if (!c->isVisible()) continue;
            if (c->getPanel()) top_panel(c->getPanel());

            int finalW, finalH;
            if (m_orientation == HORIZONTAL) {
                finalW = c->getCalculatedWidth(unitSpace, layoutAvailW);
                if (finalW > layoutAvailW) finalW = layoutAvailW;
                if ((int)i == last_index && layoutAvailW > finalW) finalW = layoutAvailW;

                finalH = c->getCalculatedHeight(0, layoutAvailH); // 교차축은 유지
                if (finalH > layoutAvailH) finalH = layoutAvailH;

                layoutAvailW -= finalW; // 주축 여유 공간 차감
            } else {
                finalW = c->getCalculatedWidth(0, layoutAvailW); // 교차축은 유지
                if (finalW > layoutAvailW) finalW = layoutAvailW;

                finalH = c->getCalculatedHeight(unitSpace, layoutAvailH);
                if (finalH > layoutAvailH) finalH = layoutAvailH;
                if ((int)i == last_index && layoutAvailH > finalH) finalH = layoutAvailH;

                layoutAvailH -= finalH; // 주축 여유 공간 차감
            }

            LOG_DEBUG << "LinearLayout: requestLayout child: " << m_name << ", " << c->getName() << " curY: " << curY << " curX: " << curX << " finalW: " << finalW << " finalH: " << finalH;
            auto group = std::dynamic_pointer_cast<ViewGroup>(c);
            if (group) {
                group->requestLayout(curY, curX, finalW, finalH);
            } else {
                c->setRect(curY, curX, finalW, finalH);
            }

            if (m_orientation == HORIZONTAL) {
                curX += finalW;
                contentW += finalW;
                contentH = std::max(contentH, finalH);
            } else {
                curY += finalH;
                contentH += finalH;
                contentW = std::max(contentW, finalW);
            }
        }

        // 5. 이 레이아웃 자신의 최종 크기 확정 및 윈도우 갱신
        int new_W = (m_originalRect.w == MATCH_PARENT || m_originalRect.w == 0) ? mW : contentW + m_padding_left + m_padding_right;
        int new_H = (m_originalRect.h == MATCH_PARENT || m_originalRect.h == 0) ? mH : contentH + m_padding_top + m_padding_bottom;
        
        if (m_originalRect.w > 0) new_W = m_originalRect.w;
        if (m_originalRect.h > 0) new_H = m_originalRect.h;

        LOG_DEBUG << "LinearLayout: calculateDimensions child: " << m_name << " new_W: " << new_W << " new_H: " << new_H;

        setRect(sY, sX, new_W, new_H);
    }


private:
    Orientation m_orientation;
};

class PopupLayout : public LinearLayout {
public:
    PopupLayout(std::string un, int y, int x) : LinearLayout(un, VERTICAL) {
        setBorder(true);
        setPadding(1, 1, 1, 1);
        setOriginalRect(y, x, WRAP_CONTENT, WRAP_CONTENT);
    }

    bool dispatchMouseEvent(MEVENT* ev, WINDOW* parentWin) override {
        if (!m_visible || !m_win) return false;

        if (wenclose(m_win, ev->y, ev->x)) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if ((*it)->dispatchMouseEvent(ev, m_win)) { return true; }
            }
            return onMouseEvent(ev);
        }else{
            setFocused(false);
            return false;
        }
    }
};

class LogView : public View {
public:
    LogView(std::string title) { m_title = title; }

    void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) override {
        
        m_calW = std::max((int)m_title.length()+1, 10+m_padding_left+m_padding_right); 
        m_calH = std::min(5+m_padding_top+m_padding_bottom, maxHeight);
    }

    void onDraw(View* parentView) override {
        drawBackground();
        
        int max_lines = m_rect.h - m_padding_top - m_padding_bottom;
        int log_count = (int)m_logs.size();
        int offset = (log_count > max_lines) ? log_count - max_lines : 0;
        for (int i = 0; i < max_lines && (i + offset) < log_count; ++i) {
            mvwprintw(m_win, m_padding_top + i, m_padding_left, "%s", m_logs[i + offset].c_str());
        }
    }
    void log(std::string msg) { m_logs.push_back(msg); if (m_logs.size() > 100) m_logs.pop_front(); }
private:
    std::deque<std::string> m_logs;
};

class Label : public View {
public:
    Label(std::string text) : m_text(text) {
        tokenize();
        calculateDimensions();
    }

    void tokenize() {
        m_lines.clear();
        std::string line;
        std::stringstream ss(m_text);
        while (std::getline(ss, line)) {
            m_lines.push_back(line);
        }
        if (m_lines.empty() && !m_text.empty()) m_lines.push_back(m_text);
    }

    void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) override {
        int maxW = 0;
        for (const auto& line : m_lines) {
            maxW = std::max(maxW, (int)line.length());
        }
        m_calW = std::min(maxW+m_padding_left+m_padding_right, maxWidth); 
        m_calH = std::min((int)m_lines.size()+m_padding_top+m_padding_bottom, maxHeight);
    }

    void onDraw(View* parentView) override { 
        drawBackground();
        
        int curY = m_padding_top;
        if(m_rect.h - m_padding_top - m_padding_bottom > (int)m_lines.size()) {
            curY += (m_rect.h - m_padding_top - m_padding_bottom - (int)m_lines.size()) / 2;
        }
        int maxW = m_rect.w - m_padding_left - m_padding_right;

        for (auto line : m_lines) {
            if (line.length() > (size_t)maxW && maxW > 3) line = line.substr(0, maxW - 3) + "...";
            else if (line.length() > (size_t)maxW) line = line.substr(0, maxW);
            
            mvwprintw(m_win, curY++, (m_rect.w - line.length()) / 2, "%s", line.c_str());
            if (curY >= m_rect.h-m_padding_bottom) break;
        }
    }
protected:
    std::string m_text;
    std::vector<std::string> m_lines;
};

class Button : public Label {
public:
    Button(std::string label) : Label(label) { 
        setFocusable(true); 
        setClickable(true);
    }

    void drawBackground(bool erase = true) override {
        if (!m_win) return;

        chtype attr = NUI_COLOR(COLOR_BLACK, COLOR_WHITE);
        
        if(m_isPressed) {
            attr = NUI_COLOR(COLOR_YELLOW, COLOR_RED) | A_BOLD;
        } else if(m_isFocused) {
            attr = NUI_COLOR(COLOR_BLACK, COLOR_YELLOW) | A_BOLD;
        } else {
            attr = NUI_COLOR(COLOR_BLACK, COLOR_WHITE);
        }

        if (erase) { wbkgd(m_win, attr); werase(m_win); }

        if (m_border) box(m_win, 0, 0);
        if (!m_title.empty()) mvwprintw(m_win, 0, 1, "%s", m_title.c_str());
    }

    void onDraw(View* parentView) override {
        Label::onDraw(parentView);
    }
};

class InputField : public View {
public:
    InputField(std::string title, std::string hint) 
        : m_savedText(""), m_hint(hint), m_form(nullptr), m_lineCount(1) { 
        
        m_fields[0] = nullptr;
        m_fields[1] = nullptr;
        
        setTitle(title);
        setPadding(1, 1, 1, 1);
        setBorder(true);
        setFocusable(true); 
        setClickable(true);
        setEditable(true);
    }
    
    virtual ~InputField() {
        if (m_form) {
            unpost_form(m_form);
            free_form(m_form);
        }
        if (m_fields[0]) free_field(m_fields[0]);
        
    }

    void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) override {

        if(m_savedText.empty()) {
            m_savedText=m_hint;
        }
        int width = std::max((int)m_savedText.length(), std::max((int)m_title.length(), (int)m_hint.length()));

        m_calW = std::min(width+m_padding_left+m_padding_right, maxWidth); 
        m_calH = std::min(m_lineCount+m_padding_top+m_padding_bottom, maxHeight);
    }
    
    void setHint(const std::string& hint) { m_hint = hint; }
    void setSavedText(const std::string& savedText) { 
        m_savedText = savedText; 
        if (m_form) {
            unpost_form(m_form);
            set_field_buffer(m_fields[0], 0, m_savedText.c_str()); 
        }
    }
    

    virtual void setFocused(bool f) override { 
        View::setFocused(f); 
        if(!f && m_form && !isEditing()) {
            //LOG_INFO << m_name << " Unpost form============================================";
            unpost_form(m_form);
            set_field_buffer(m_fields[0], 0, m_savedText.c_str());  
        }//else LOG_INFO << m_name << " Unpost form===============XXXXXXXXXXXXXXXX=====================" << isEditing() << " " << f;
    }
    
    void setEditing(bool editing) override {
        View::setEditing(editing);
        if (m_form) {
            if (!editing) {
                m_savedText = getTextContent();
                // 편집 종료 후 폼을 리프레시하여 화면에 정상 표시되도록 함
                unpost_form(m_form);
                set_field_buffer(m_fields[0], 0, m_savedText.c_str());
            } else {
                // 편집 시작 시점에만 커서를 텍스트 끝으로 이동
                form_driver(m_form, REQ_END_LINE);
            }
        }
    }

    virtual bool onMouseEvent(MEVENT* ev) override {
        if(m_form && !isEditing()) {
            //LOG_INFO << "Unpost form============================================";
            unpost_form(m_form);
            set_field_buffer(m_fields[0], 0, m_savedText.c_str());  
        }//else LOG_INFO << "Unpost form=======XXXXX==============XXXXX=======================";
        return View::onMouseEvent(ev);

        // if(View::onMouseEvent(ev)) {
        //     if(ev->bstate & BUTTON1_CLICKED) {
        //         //if(m_form) {unpost_form(m_form);}
        //     }
        //     return true;
        // }
        // return false;
    }

    virtual void setRect(int y, int x, int w, int h) override { 
        View::setRect(y, x, w, h); 

        if (m_form) {
            unpost_form(m_form);
            free_form(m_form);
            m_form = nullptr;
        }
        
        int curY = m_padding_top;
        int maxW = m_rect.w - m_padding_left - m_padding_right;
        
        
        if (m_fields[0]) free_field(m_fields[0]);
        
        // 물리적 너비는 maxW로 설정하고, O_STATIC을 꺼서 수평 스크롤 허용
        m_fields[0] = new_field(m_lineCount, maxW, 0, 0, 0, 0);
        field_opts_off(m_fields[0], O_AUTOSKIP);
        field_opts_off(m_fields[0], O_BLANK);
        field_opts_off(m_fields[0], O_STATIC); // 물리적 너비보다 긴 입력 및 수평 스크롤 허용
        
        set_field_back(m_fields[0], NUI_COLOR(COLOR_WHITE, COLOR_GREEN) | A_UNDERLINE);
        set_field_fore(m_fields[0], NUI_COLOR(COLOR_BLACK, COLOR_BLUE));
        set_field_buffer(m_fields[0], 0, m_savedText.c_str());  
        m_form = new_form(m_fields);
        set_form_win(m_form, m_win);
        if (m_subWin) delwin(m_subWin);
        // 화면에 보이는 영역은 원래대로 maxW로 제한
        m_subWin = derwin(m_win, 1, maxW, m_padding_top, m_padding_left);
        set_form_sub(m_form, m_subWin); 
            
        
    }

    void onDraw(View* parentView) override { 
        drawBackground();
        if (!m_win) return;

        //LOG_INFO << "Post form##########################=======================";
        post_form(m_form);

        if (m_isEditing) {
            //set_field_back(m_fields[0], NUI_COLOR(COLOR_YELLOW, COLOR_WHITE) | A_UNDERLINE);
            set_field_fore(m_fields[0], NUI_COLOR(COLOR_BLACK, COLOR_BLUE));
            LOG_DEBUG << "InputField: onDraw m_isEditing=true";
            showCursor(true);
            pos_form_cursor(m_form);
        } else {
            set_field_back(m_fields[0], NUI_COLOR(COLOR_WHITE, COLOR_GREEN) | A_UNDERLINE);
        }

    }

    bool onKeyEvent(int ch) override {
        if (!m_form) return false;
        LOG_INFO << m_name << " onKeyEvent ch: " << ch;
        int res = E_OK;
        switch(ch) {
            case KEY_BACKSPACE:
            case 127:
            case '\b':
                res = form_driver(m_form, REQ_DEL_PREV);
                break;
            case KEY_LEFT:
            LOG_INFO << "KEY_LEFT";
                res = form_driver(m_form, REQ_PREV_CHAR);
                break;
            case KEY_RIGHT:
            LOG_INFO << "KEY_RIGHT";
                res = form_driver(m_form, REQ_NEXT_CHAR);
                break;
            case KEY_HOME:
                res = form_driver(m_form, REQ_BEG_LINE);
                break;
            case KEY_END:
                res = form_driver(m_form, REQ_END_LINE);
                break;
            case KEY_DC:
                res = form_driver(m_form, REQ_DEL_CHAR);
                break;
            case KEY_ENTER:
            case '\n':
            case '\r':
                if(m_lineCount > 1) {
                    res = form_driver(m_form, REQ_NEW_LINE);
                } else {
                    setEditing(false); 
                }
                break;
            default:
                if (isprint(ch)) {
                    LOG_DEBUG << "InputField: onKeyEvent isprint(ch) true ch: " << ch;
                    res = form_driver(m_form, ch);
                } else {
                    LOG_DEBUG << "InputField: onKeyEvent isprint(ch) false ch: " << ch;
                    return false;
                }
                break;
        }
        return (res == E_OK);
    }

    std::string getTextContent() const {
        if (!m_fields[0] || !m_form) return "";
        form_driver(m_form, REQ_VALIDATION);
        char* buf = field_buffer(m_fields[0], 0);
        if (!buf) return "";
        std::string res = buf;
        // Trim trailing spaces
        size_t last = res.find_last_not_of(' ');
        if (last == std::string::npos) return "";
        return res.substr(0, last + 1);
    }

private:
    std::string m_savedText;
    std::string m_hint;
    FIELD* m_fields[2];
    FORM* m_form;
    int m_lineCount;
    //WINDOW* m_formSubWin;
};


class Spinner : public View {
public:
    Spinner(std::string title, std::vector<std::string> items, std::function<void(int)> onSelected) 
        : m_items(items), m_selectedIndex(0), m_onSelected(onSelected) {
        setTitle(title);
        setFocusable(true);
        setClickable(true);
        setBorder(true);
        setPadding(1, 1, 1, 1);
    }

    void calculateDimensions(int maxWidth = 9999, int maxHeight = 9999) override {

        int maxItemLen = 0;
        for (const auto& item : m_items) {
            maxItemLen = std::max(maxItemLen, (int)item.length());
        }
        int width = std::max((int)m_title.length(), maxItemLen + 2); 

        m_calW = std::min(width+m_padding_left+m_padding_right, maxWidth); 
        m_calH = 1;//std::min((int)m_items.size()+m_padding_top+m_padding_bottom, maxHeight);
    }

    void onDraw(View* parentView) override {
        drawBackground();
        if (!m_win) return;

        std::string text = m_items.empty() ? "None" : m_items[m_selectedIndex];
        
        int innerW = m_rect.w - m_padding_left - m_padding_right;
        // 화살표 크기(1)와 최소 공백(1)을 뺀 나머지 공간이 텍스트 공간
        int maxTextLen = innerW - 2;
        if (maxTextLen < 0) maxTextLen = 0;

        std::string displayText = text;
        if ((int)displayText.length() > maxTextLen) {
            if (maxTextLen >= 3) {
                displayText = displayText.substr(0, maxTextLen - 3) + "...";
            } else {
                displayText = displayText.substr(0, maxTextLen);
            }
        }

        // %-*s 를 사용해 maxTextLen 만큼 좌측 정렬(우측 공백 채움) 후 화살표 출력
        mvwprintw(m_win, m_padding_top, m_padding_left, "%-*s ▼", maxTextLen, displayText.c_str());
    }

    void setSelectedIndex(int index) {
        if (index >= 0 && index < (int)m_items.size()) {
            m_selectedIndex = index;
            if (m_onSelected) m_onSelected(index);
        }
    }

    int getSelectedIndex() const { return m_selectedIndex; }
    std::string getSelectedItem() const { 
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size()) return m_items[m_selectedIndex];
        return "";
    }
    const std::vector<std::string>& getItems() const { return m_items; }

    bool onMouseEvent(MEVENT* ev) override {
        bool handled = View::onMouseEvent(ev);
        if (ev->bstate & BUTTON1_CLICKED) {
            showPopup(ev);
            return true;
        }
        return handled;
    }

    void showPopup(MEVENT* ev) {
        int pW = m_rect.w;
        int pH = std::min((int)m_items.size() + 2, 10);
        int pY = ev->y;
        int pX = ev->x;

        auto popup = std::make_shared<PopupLayout>("spinner_popup", pY, pX);
        
        for (size_t i = 0; i < m_items.size(); ++i) {
            auto itemBtn = std::make_shared<Button>(m_items[i]);
            itemBtn->setSize(WRAP_CONTENT, WRAP_CONTENT);
            itemBtn->setOnClickListener([this, i](){
                LOG_DEBUG << "Spinner: on click index: " << i;
                setSelectedIndex((int)i);
                requestClosePopup();
            });
            popup->addView(itemBtn);
        }
        requestPopup(popup);
    }


private:
    std::vector<std::string> m_items;
    int m_selectedIndex;
    std::function<void(int)> m_onSelected;
};

} // namespace nui



#include "IVissClient.h"
#include "GrpcVissClient.h"

class HttpsVissClient : public IVissClient {
public:
    void start(const std::string& address) override { if(m_onEvent) m_onEvent("HttpsVissClient started at " + address); }
    void stop() override { if(m_onEvent) m_onEvent("HttpsVissClient stopped"); }
    void sendRequest(const std::string& jsonReq) override { if(m_onResponse) m_onResponse("HTTPS Response for: " + jsonReq); }
};

class WssVissClient : public IVissClient {
public:
    void start(const std::string& address) override { if(m_onEvent) m_onEvent("WssVissClient started at " + address); }
    void stop() override { if(m_onEvent) m_onEvent("WssVissClient stopped"); }
    void sendRequest(const std::string& jsonReq) override { if(m_onResponse) m_onResponse("WSS Response for: " + jsonReq); }
};

class UdsVissClient : public IVissClient {
public:
    void start(const std::string& address) override { if(m_onEvent) m_onEvent("UdsVissClient started at " + address); }
    void stop() override { if(m_onEvent) m_onEvent("UdsVissClient stopped"); }
    void sendRequest(const std::string& jsonReq) override { if(m_onResponse) m_onResponse("UDS Response for: " + jsonReq); }
};

class MqttVissClient : public IVissClient {
public:
    virtual ~MqttVissClient() { FileLogger::getInstance().write("MqttVissClient destroyed"); }
    void start(const std::string& address) override { if(m_onEvent) m_onEvent("MqttVissClient started at " + address); }
    void stop() override { if(m_onEvent) m_onEvent("MqttVissClient stopped"); }
    void sendRequest(const std::string& jsonReq) override { if(m_onResponse) m_onResponse("MQTT Response for: " + jsonReq); }
};

class VissClientManager {
public:
    void sendRequest(const std::string& name, 
                     const std::string& protocol, 
                     const std::string& address, 
                     const std::string& json,
                     std::function<void(const std::string&)> onResponse,
                     std::function<void(const std::string&)> onEvent,
                     std::function<void(const std::string&, const std::string&)> onSubscription) {
        
        std::string key = name + "_" + protocol;
        std::string configInfo = protocol + "|" + address;

        // 1. 클라이언트 확보 혹은 생성
        std::shared_ptr<IVissClient> client = getOrCreateClient(name, protocol);
        if (!client) return;

        // 2. 연결 상태 및 설정 변경 확인
        if (m_activeConfigs[name] != configInfo) {
            LOG_INFO << "Manager: Re-initializing client " << name << " with " << configInfo;
            client->stop();
            
            // 콜백 설정
            client->setResponseCallback(onResponse);
            client->setEventCallback(onEvent);
            client->setSubscriptionCallback(onSubscription);
            
            client->start(address);
            m_activeConfigs[name] = configInfo;
        }

        // 3. 메시지 전송
        client->sendRequest(json);
    }

    void removeClient(const std::string& name) {
        for (auto it = m_clients.begin(); it != m_clients.end(); ) {
            if (it->first.find(name + "_") == 0) {
                it->second->stop();
                it = m_clients.erase(it);
            } else {
                ++it;
            }
        }
        m_activeConfigs.erase(name);
    }

    void stopAll() {
        for (auto& pair : m_clients) {
            if (pair.second) pair.second->stop();
        }
        m_clients.clear();
        m_activeConfigs.clear();
    }

private:
    std::shared_ptr<IVissClient> getOrCreateClient(const std::string& name, const std::string& protocol) {
        std::string key = name + "_" + protocol;
        if (m_clients.count(key)) return m_clients[key];

        // 같은 이름의 다른 프로토콜이 있다면 정리
        for (auto it = m_clients.begin(); it != m_clients.end(); ) {
            if (it->first.find(name + "_") == 0) {
                it->second->stop();
                it = m_clients.erase(it);
            } else {
                ++it;
            }
        }

        std::shared_ptr<IVissClient> client;
        if (protocol == "WSS") client = std::make_shared<WssVissClient>();
        else if (protocol == "HTTPS") client = std::make_shared<HttpsVissClient>();
        else if (protocol == "MQTT") client = std::make_shared<MqttVissClient>();
        else if (protocol == "gRPC") client = std::make_shared<GrpcVissClient>();
        else if (protocol == "UDS") client = std::make_shared<UdsVissClient>();
        
        if (client) m_clients[key] = client;
        return client;
    }

    std::map<std::string, std::shared_ptr<IVissClient>> m_clients;
    std::map<std::string, std::string> m_activeConfigs; // 클라이언트별 현재 활성화된 "프로토콜|주소" 정보
};

class Application {
public:
    Application():m_popup(nullptr) {
        LOG_INFO << "Application constructor";
        setlocale(LC_ALL, "");
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
		
        for (int f = 0; f < 8; f++) {
            for (int b = 0; b < 8; b++) {
                init_pair(f * 8 + b + 1, f, b);
            }
        }
        curs_set(0); 
        set_escdelay(200);
        m_inputMgr.setMode(nui::InputManager::RAW_MODE); 
        nodelay(stdscr, TRUE); // non-blocking getch
        refresh();
        int my, mx; getmaxyx(stdscr, my, mx);
        m_root = std::make_shared<nui::LinearLayout>("root", nui::VERTICAL);
        //m_root->setBorder(true);
        m_root->setSize(MATCH_PARENT, MATCH_PARENT);
        m_systemLog = std::make_shared<nui::LogView>("System Global Log");
        m_systemLog->setName("m_systemLog");
        m_systemLog->setBorder(true);
        m_systemLog->setPadding(1, 1, 1, 1);
        m_systemLog->setWeight(2);
        m_systemLog->setSize(MATCH_PARENT, MATCH_PARENT);
    
        // FileLogger와 UI 로그 연결
        FileLogger::getInstance().setCallback([this](const std::string& msg){
            if (m_systemLog) {
                m_systemLog->log(msg);
                //m_root->onDraw(nullptr);
                //update_panels();
                //doupdate();
            }
        });

        setContentView(); 
    }
    std::shared_ptr<nui::View> client(int index) {
        int my, mx; getmaxyx(stdscr, my, mx);
        std::string clientName = "client_" + std::to_string(index);
        auto clientLayout = std::make_shared<nui::LinearLayout>(clientName, nui::VERTICAL);
        clientLayout->setSize(0, WRAP_CONTENT);
        clientLayout->setWeight(1);
        clientLayout->setPadding(1, 1, 1, 1); clientLayout->setBorder(true);
        clientLayout->setTitle("CLIENT " + std::to_string(index));

        // transport selector & address input
        auto transportLayout = std::make_shared<nui::LinearLayout>("transport", nui::HORIZONTAL);
        //transportLayout->setBorder(true);
        //transportLayout->setPadding(1,1,1,1);
        transportLayout->setSize(MATCH_PARENT, WRAP_CONTENT);
        
        auto transportSpinner = std::make_shared<nui::Spinner>("Transport", 
            std::vector<std::string>{"gRPC", "WSS", "HTTPS", "MQTT", "DDS", "UDS"},
            [this, clientName](int idx) { 
                std::vector<std::string> transports = {"gRPC", "WSS", "HTTPS", "MQTT", "DDS", "UDS"};
                std::string t = transports[idx];
                LOG_INFO << clientName << " Transport selected: " << t; 
            }
        );
        transportSpinner->setName("transportSpinner");
        transportSpinner->setSize(0, WRAP_CONTENT);
        transportSpinner->setWeight(1);

        auto addressInput = std::make_shared<nui::InputField>("Server Address", "127.0.0.1:5443");
        addressInput->setName("addressInput");
        addressInput->setSize(0, WRAP_CONTENT);
        addressInput->setWeight(4);
        
        transportLayout->addView(transportSpinner);
        transportLayout->addView(addressInput);
        clientLayout->addView(transportLayout);

        // action selector & btn
        auto actionLayout = std::make_shared<nui::LinearLayout>("action", nui::HORIZONTAL);
        actionLayout->setSize(MATCH_PARENT, WRAP_CONTENT);

        auto actionSpinner = std::make_shared<nui::Spinner>("Action", 
            std::vector<std::string>{"get", "set", "subscribe", "unsubscribe"},
            [this](int idx){ m_systemLog->log("Action changed to " + std::to_string(idx)); }
        );
        actionSpinner->setName(clientName + "_actionSpinner");
        actionSpinner->setSize(0, WRAP_CONTENT);
        actionSpinner->setWeight(1);
        actionLayout->addView(actionSpinner);

        auto valueInput = std::make_shared<nui::InputField>("Value", "");
        valueInput->setName(clientName + "_valueInput");
        valueInput->setSize(0, WRAP_CONTENT);
        valueInput->setWeight(4);
        actionLayout->addView(valueInput);

        clientLayout->addView(actionLayout);

        auto pathInput = std::make_shared<nui::InputField>("Path", "Vehicle.Speed");
        pathInput->setName(clientName + "_pathInput");
        pathInput->setSize(MATCH_PARENT, WRAP_CONTENT);
        clientLayout->addView(pathInput);


        auto filterLayout = std::make_shared<nui::LinearLayout>("filter", nui::VERTICAL);
        filterLayout->setSize(MATCH_PARENT, WRAP_CONTENT);

        auto variantLayout = std::make_shared<nui::LinearLayout>("variant1", nui::HORIZONTAL);
        variantLayout->setSize(MATCH_PARENT, WRAP_CONTENT);

        auto paramInput = std::make_shared<nui::InputField>("Parameter", "");
        paramInput->setName(clientName + "_paramInput");
        paramInput->setSize(0, WRAP_CONTENT);
        paramInput->setWeight(4);

        auto variantSpinner = std::make_shared<nui::Spinner>("Variant", 
            std::vector<std::string>{"None","paths","timebased","range","change","curvelog","history","metadata"},
            [this, paramInput](int idx){ 
                std::vector<std::string> hints = {
                    "Parameter", 
                    "[\"Path1\",\"Path2\", ...]", 
                    "\"Period(ms)\"", 
                    "{\"logic-op\":\"gt\",\"boundary\":\"49\"}", 
                    "{\"logic-op\":\"ne\",\"diff\":\"0\"}", 
                    "{\"maxerr\":\"0.5\",\"bufsize\":\"100\"}", 
                    "PdddDThhHmmMssS ex)P2DT12H18M20S", 
                    "\"Depth\"  //0: no limit"
                };
                if (idx >= 0 && idx < (int)hints.size()) {
                    paramInput->setSavedText(hints[idx]);
                }
                m_systemLog->log("Variant changed to " + std::to_string(idx)); 
            }
        );
        variantSpinner->setName(clientName + "_variantSpinner");
        variantSpinner->setSize(0, WRAP_CONTENT);
        variantSpinner->setWeight(1);
        
        variantLayout->addView(variantSpinner);
        variantLayout->addView(paramInput);

        auto variantLayout2 = std::make_shared<nui::LinearLayout>("variant2", nui::HORIZONTAL);
        variantLayout2->setSize(MATCH_PARENT, WRAP_CONTENT);
        
        auto paramInput2 = std::make_shared<nui::InputField>("Parameter", "");
        paramInput2->setName(clientName + "_paramInput2");
        paramInput2->setSize(0, WRAP_CONTENT);
        paramInput2->setWeight(4);

        auto variantSpinner2 = std::make_shared<nui::Spinner>("Variant", 
            std::vector<std::string>{"None","paths","timebased","range","change","curvelog","history","metadata"},
            [this, paramInput2](int idx){ 
                std::vector<std::string> hints = {
                    "Parameter", 
                    "[\"Path1\",\"Path2\", ...]", 
                    "\"Period(ms)\"", 
                    "{\"logic-op\":\"gt\",\"boundary\":\"49\"}", 
                    "{\"logic-op\":\"ne\",\"diff\":\"0\"}", 
                    "{\"maxerr\":\"0.5\",\"bufsize\":\"100\"}", 
                    "PdddDThhHmmMssS ex)P2DT12H18M20S", 
                    "\"Depth\"  //0: no limit"
                };
                if (idx >= 0 && idx < (int)hints.size()) {
                    paramInput2->setSavedText(hints[idx]);
                }
                m_systemLog->log("Variant2 changed to " + std::to_string(idx)); 
            }
        );
        variantSpinner2->setName(clientName + "_variantSpinner2");
        variantSpinner2->setSize(0, WRAP_CONTENT);
        variantSpinner2->setWeight(1);

        variantLayout2->addView(variantSpinner2);
        variantLayout2->addView(paramInput2);

        filterLayout->addView(variantLayout);
        filterLayout->addView(variantLayout2);

        clientLayout->addView(filterLayout);

        auto resView = std::make_shared<nui::LogView>("Response Area");
        resView->setName(clientName + "_resView");
        resView->setBorder(true);
        resView->setPadding(1, 1, 1, 1);
        resView->setSize(MATCH_PARENT, 10);
        clientLayout->addView(resView);

        auto eventView = std::make_shared<nui::LogView>("Event Area");
        eventView->setName(clientName + "_eventView");
        eventView->setBorder(true);
        eventView->setPadding(1, 1, 1, 1);
        eventView->setSize(MATCH_PARENT, 10); 
        clientLayout->addView(eventView);

        auto btnLayout = std::make_shared<nui::LinearLayout>("btnLayout", nui::HORIZONTAL);
        btnLayout->setSize(MATCH_PARENT, WRAP_CONTENT);

        auto makeJsonRequest = [this, clientName]() -> std::string {
            auto actionSpinner = std::dynamic_pointer_cast<nui::Spinner>(m_root->findByName(clientName + "_actionSpinner"));
            auto pathInput = std::dynamic_pointer_cast<nui::InputField>(m_root->findByName(clientName + "_pathInput"));
            auto valueInput = std::dynamic_pointer_cast<nui::InputField>(m_root->findByName(clientName + "_valueInput"));
            auto variantSpinner = std::dynamic_pointer_cast<nui::Spinner>(m_root->findByName(clientName + "_variantSpinner"));
            auto paramInput = std::dynamic_pointer_cast<nui::InputField>(m_root->findByName(clientName + "_paramInput"));
            auto variantSpinner2 = std::dynamic_pointer_cast<nui::Spinner>(m_root->findByName(clientName + "_variantSpinner2"));
            auto paramInput2 = std::dynamic_pointer_cast<nui::InputField>(m_root->findByName(clientName + "_paramInput2"));

            if (!actionSpinner || !pathInput) return "";

            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            doc.AddMember("action", rapidjson::Value(actionSpinner->getSelectedItem().c_str(), allocator).Move(), allocator);
            doc.AddMember("path", rapidjson::Value(pathInput->getTextContent().c_str(), allocator).Move(), allocator);
            
            std::string val = valueInput ? valueInput->getTextContent() : "";
            if (!val.empty()) {
                doc.AddMember("value", rapidjson::Value(val.c_str(), allocator).Move(), allocator);
            }

            auto buildFilter = [&](std::shared_ptr<nui::Spinner> s, std::shared_ptr<nui::InputField> p) -> rapidjson::Value {
                if (!s || s->getSelectedIndex() <= 0) return rapidjson::Value();
                std::string type = s->getSelectedItem();
                std::string param = p ? p->getTextContent() : "";
                
                rapidjson::Value filterObj(rapidjson::kObjectType);
                filterObj.AddMember("variant", rapidjson::Value(type.c_str(), allocator).Move(), allocator);
                
                rapidjson::Document pDoc;
                pDoc.Parse(param.c_str());
                if (!pDoc.HasParseError()) {
                    filterObj.AddMember("parameter", rapidjson::Value().CopyFrom(pDoc, allocator), allocator);
                } else {
                    filterObj.AddMember("parameter", rapidjson::Value("JSON Parse Error. Check Input.", allocator).Move(), allocator);
                }
                return filterObj;
            };

            rapidjson::Value f1 = buildFilter(variantSpinner, paramInput);
            rapidjson::Value f2 = buildFilter(variantSpinner2, paramInput2);

            if (!f1.IsNull() || !f2.IsNull()) {
                if (!f1.IsNull() && !f2.IsNull()) {
                    rapidjson::Value filters(rapidjson::kArrayType);
                    filters.PushBack(f1, allocator);
                    filters.PushBack(f2, allocator);
                    doc.AddMember("filter", filters, allocator);
                } else {
                    doc.AddMember("filter", !f1.IsNull() ? f1 : f2, allocator);
                }
            }

            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::string requestId = clientName + "-" + std::to_string(timestamp);
            doc.AddMember("requestId", rapidjson::Value(requestId.c_str(), allocator).Move(), allocator);
            

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
            return buffer.GetString();
        };

        auto btnSend = std::make_shared<nui::Button>("SEND");
        btnSend->setName("btnSend");
        btnSend->setRect(0, 0, 10, 3);
        btnSend->setOnClickListener([this, clientName, transportSpinner, addressInput, makeJsonRequest]() { 
            std::vector<std::string> transports = {"gRPC", "WSS", "HTTPS", "MQTT", "DDS", "UDS"};
            std::string protocol = transports[transportSpinner->getSelectedIndex()];
            std::string address = addressInput->getTextContent();
            std::string json = makeJsonRequest();

            m_clientMgr.sendRequest(
                clientName, protocol, address, json,
                [this, clientName](const std::string& res) {
                    auto resView = std::dynamic_pointer_cast<nui::LogView>(m_root->findByName(clientName + "_resView"));
                    if(resView) {
                        resView->log(res);
                        //m_root->onDraw(nullptr);
                        //update_panels();
                        //doupdate();
                    }
                },
                [this, clientName](const std::string& ev) {
                    auto evView = std::dynamic_pointer_cast<nui::LogView>(m_root->findByName(clientName + "_eventView"));
                    if(evView) {
                        evView->log(ev);
                        //m_root->onDraw(nullptr);
                        //update_panels();
                        //doupdate();
                    }
                },
                [this, clientName](const std::string& path, const std::string& subId) {
                    auto resView = std::dynamic_pointer_cast<nui::LogView>(m_root->findByName(clientName + "_resView"));
                    if(resView) resView->log("[SUCCESS] New Subscription: " + path + " -> " + subId);
                }
            );
        });

        btnLayout->addView(btnSend);

        auto btnViewJson = std::make_shared<nui::Button>("View JSON");
        btnViewJson->setName("btnViewJson");
        btnViewJson->setRect(0, 0, 10, 3);
        btnViewJson->setOnClickListener([this, clientName, makeJsonRequest]() { 
            std::string json = makeJsonRequest();
            auto resView = std::dynamic_pointer_cast<nui::LogView>(m_root->findByName(clientName + "_resView"));
            if (resView) {
                resView->log("[View JSON] Generated Request Body:");
                resView->log(json);
            }
        });
        btnLayout->addView(btnViewJson);
        
        clientLayout->addView(btnLayout);
        return clientLayout;
    }
    void setContentView() {
        int my, mx; getmaxyx(stdscr, my, mx);
        m_root->setPopupHandler([this](std::shared_ptr<nui::View> popup){
            if (popup) {
                int y, x, w, h; getmaxyx(stdscr, y, x);
                m_popup = std::dynamic_pointer_cast<nui::PopupLayout>(popup);
                if (m_popup) {
                    int py = m_popup->getOriginalRect().y;
                    int px = m_popup->getOriginalRect().x;
                    m_popup->updateLayout(py, px, x - px, y - py);
                }
            } else {
                m_popup = nullptr;
            }
        });

        auto menu = std::make_shared<nui::LinearLayout>("menu", nui::HORIZONTAL);
        //menu->setMaxSize(0, 5);
        menu->setPadding(1, 1, 1, 1);
        menu->setBorder(true);
        menu->setSize(MATCH_PARENT, WRAP_CONTENT);

        auto btnAdd = std::make_shared<nui::Button>("Add Client");
        btnAdd->setName("btnadd");
        btnAdd->setBorder(true);
        btnAdd->setPadding(1, 1, 1, 1);
        btnAdd->setWeight(1);
        btnAdd->setSize(0, WRAP_CONTENT);
        btnAdd->setOnClickListener([this](){ addClient(); });
        menu->addView(btnAdd);
        
        auto btnRem = std::make_shared<nui::Button>("Remove Client");
        btnRem->setName("btnRem");
        btnRem->setWeight(1);
        btnRem->setBorder(true);
        btnRem->setPadding(1, 1, 1, 1);
        btnRem->setSize(0, WRAP_CONTENT);
        btnRem->setOnClickListener([this](){ removeClient(); });
        menu->addView(btnRem);
        
        auto btnQuit = std::make_shared<nui::Button>("Quit");
        btnQuit->setName("btnQuit");
        btnQuit->setWeight(1);
        btnQuit->setSize(0, WRAP_CONTENT);
        btnQuit->setBorder(true);
        btnQuit->setPadding(1, 1, 1, 1);
        btnQuit->setOnClickListener([this](){ m_running = false; });
        menu->addView(btnQuit);

        m_clientContainer = std::make_shared<nui::LinearLayout>("clientContainer", nui::HORIZONTAL);
        m_clientContainer->setSize(MATCH_PARENT, WRAP_CONTENT);
        
        
        auto firstClientLayout = client(1);
        m_clientContainer->addView(firstClientLayout);
        m_clientLayouts.push_back(firstClientLayout);

        m_root->addView(menu);
        m_root->addView(m_clientContainer);
        m_root->addView(m_systemLog);




        m_root->updateLayout(0,0,mx,my);
    }

    void addClient() { 
        if (m_clientContainer) {
            int nextIdx = m_clientLayouts.size() + 1;
            std::string clientName = "client_" + std::to_string(nextIdx);
            
            auto layout = client(nextIdx);
            m_clientContainer->addView(layout);
            m_clientLayouts.push_back(layout);
            
            int my, mx; getmaxyx(stdscr, my, mx);
            m_root->updateLayout(0, 0, mx, my);
            LOG_INFO << "Added UI Layout: " << clientName;
        }
    }
    
    void removeClient() { 
        if (m_clientContainer && !m_clientLayouts.empty()) {
            auto lastLayout = m_clientLayouts.back();
            std::string clientName = lastLayout->getName();
            
            m_clientMgr.removeClient(clientName);
            
            m_clientLayouts.pop_back();

            m_clientContainer->removeAllViews();
            for (auto& layout : m_clientLayouts) {
                m_clientContainer->addView(layout);
            }
            
            int my, mx; getmaxyx(stdscr, my, mx);
            m_root->updateLayout(0, 0, mx, my);
            LOG_INFO << "Removed " << clientName;
        }
    }
    void run() {
        m_root->onDraw(nullptr);
        update_panels();
        doupdate();

        while (m_running) {
            m_inputMgr.poll();
            
            nui::Event ev;
            while (m_inputMgr.popEvent(ev)) {
                if (ev.type == nui::Event::KEY) {
                    int ch = ev.code;
                    if (ch == KEY_RESIZE) {
                        int my, mx; getmaxyx(stdscr, my, mx);
                        resizeterm(my, mx);
                        flushinp();
                        m_root->updateLayout(0, 0, mx, my);
                        m_root->onDraw(nullptr);
                        if (m_popup) m_popup->onDraw(nullptr);
                        update_panels();
                        doupdate();
                        continue;
                    }
                    //if (ch == 'q' || ch == 'Q') { m_running = false; break; }
                    if (ch == 27) { // Escape - 모드 전환 단축키로 활용 가능
                        // m_inputMgr.setMode(m_inputMgr.getMode() == nui::InputManager::RAW_MODE ? nui::InputManager::NCURSES_MODE : nui::InputManager::NCURSES_MODE);
                    }
                    if (ch == '\t') { m_root->requestFocusNext(); continue; }
                    
                    if (m_popup) m_popup->dispatchKeyEvent(ch);
                    else m_root->dispatchKeyEvent(ch);
                } 
                else if (ev.type == nui::Event::MOUSE) {
                    MEVENT ncurseEv;
                    ncurseEv.x = ev.x;
                    ncurseEv.y = ev.y;
                    ncurseEv.bstate = ev.code;
                    
                    LOG_INFO << "Event: " << (m_inputMgr.getMode() == nui::InputManager::RAW_MODE ? "RAW" : "NCURSES") 
                             << " Mouse " << (ev.pressed ? "Dn" : "Up") << " X:" << ev.x << " Y:" << ev.y 
                             << " 0x" << std::hex << ev.code << std::dec << " C:" 
                             << ((ev.code & BUTTON1_CLICKED) ? "1Click" : ((ev.code & BUTTON1_DOUBLE_CLICKED) ? "2Click" : ((ev.code & BUTTON1_TRIPLE_CLICKED) ? "3Click" : "None")));

                    if (m_popup) {
                        if (m_popup->dispatchMouseEvent(&ncurseEv, stdscr)) { }
                        //else { m_popup = nullptr; }
                    } else {
                        m_root->dispatchMouseEvent(&ncurseEv, stdscr);
                    }
                }

                m_root->onDraw(nullptr);
                if (m_popup) m_popup->onDraw(nullptr);

                update_panels();
                doupdate();
            }
            
            usleep(10000); // CPU 점유율 조절
        }
        endwin();
    }
    void stop() { m_running = false; }
private:
    nui::InputManager m_inputMgr;
    std::shared_ptr<nui::LinearLayout> m_root;
    std::shared_ptr<nui::LinearLayout> m_clientContainer;
    std::shared_ptr<nui::PopupLayout> m_popup;
    VissClientManager m_clientMgr;
    std::vector<std::shared_ptr<nui::View>> m_clientLayouts;
    std::shared_ptr<nui::LogView> m_systemLog;
    bool m_running = true;
};


Application* g_app_ptr = nullptr;

void signal_handler(int signum) {
    LOG_WARN << "Interrupt signal (" << signum << ") received. Shutting down..." << g_app_ptr;
    if (g_app_ptr) {
        g_app_ptr->stop();
    }
}

int main() { 
    Application app; 
    
    g_app_ptr = &app;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    app.run(); 
    return 0; 
}
