#ifndef COMPLEMENTER_H
#define COMPLEMENTER_H

#include <iostream>
#include <algorithm>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <vector>

using namespace std;

class Complementer {
public:
    Complementer();
    ~Complementer();

    void setSet(set<string> &data);

    void setInput(const string& input);
    const vector<reference_wrapper<string const>>& getHints() const;
    const string& getHint(const string& ifNotFound) const;

private:
    set<string> m_data;
    //    list<string> hints;
    vector<reference_wrapper<string const>> m_hints;
    string m_hint;
};

#endif // COMPLEMENTER_H
