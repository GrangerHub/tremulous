//
// Testing Com_Parse()
//

#include <iostream>
#include <vector>

using Args = std::vector<std::string>;

struct Parser {

    Args args;

    Args& Parse(std::string src)
    {
        args.clear();
        token.clear();
        state = prev_state = START;
        scan = SCAN_START;

        for (int ch : src)
        {
            if (!process(ch))
                continue;

            args.emplace_back(token);
            token.clear();
        }

        // flush any partial states
        process(-1);

        if (token.size())
            args.emplace_back(token);

        return args;
    }

private:

    enum State {
        START,
        DQUOTE,
        ESCAPE,
        COMMENT_START,
        ONELINE_COMMENT
    };

    enum ScanState {
        SCAN_START,
        SCAN_FOUND,
        SCAN_BREAK,
        SCAN_STOP
    };

    bool process(int ch)
    {
        if (ch == -1)
        {
            if (state == COMMENT_START)
                token += '/';
            else if (state == ESCAPE)
                token += '\\';
            else
                return false;
            return true;
        }

        switch (state) {
again:
            case START:
                if (isspace(ch)) {
                    if (scan == SCAN_FOUND)
                        scan = SCAN_BREAK;
                    break;
                }
                else if (ch == '"') {
                    state = DQUOTE;
                    break;
                }
                else if (ch == '/') {
                    state = COMMENT_START;
                    break;
                }
                else if (ch == '\\') {
                    prev_state = state;
                    state = ESCAPE;
                    break;
                }

                scan = SCAN_FOUND;
                token += ch;
                break;

            // Begin comment scanner
            case COMMENT_START:
                if (ch == '/') {
                    scan = SCAN_STOP;
                    state = ONELINE_COMMENT;
                    break;
                }

                token += '/';
                state = START;
                scan = SCAN_FOUND;
                goto again;

            // Oneline comment ends the processing. We could optimize this
            case ONELINE_COMMENT:
                break;

            // String literal
            case DQUOTE:
                if (ch == '\\') {
                    prev_state = state;
                    state = ESCAPE;
                    break;
                }
                else if (ch == '"') {
                    state = START;
                    scan = SCAN_BREAK;
                    break;
                } 

                token += ch;
                break;

            case ESCAPE:
                token += ch;
                state = prev_state;
                break;
        }

        if (scan == SCAN_BREAK) {
            scan = SCAN_START;
            state = START;
            return true;
        }

        if (scan == SCAN_STOP) {
            if (!token.empty())
                return true;
            return false;
        }

        return false;
    }

    State state;
    State prev_state; // used to restore state after processing an escaped literal char. 
    ScanState scan;
    std::string token;
};
