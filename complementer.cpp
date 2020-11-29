#include "complementer.h"

Complementer::Complementer() {}

Complementer::~Complementer() {}

void Complementer::setSet(set<string>& data)
{
    m_data = data;
}

void Complementer::setInput(const string& input)
{
    m_hints.clear();

    for (const string& dataStr : m_data) {
        if (!dataStr.find(input))
            m_hints.push_back(reference_wrapper<string const>(dataStr));
    }

    if (m_hints.size() < 1 || input.size() < 1) {
        m_hint = "";
    }
    else {
        m_hint = m_hints.front();
        for (const string& str : m_hints) {
            unsigned int commonSymbols = 0;

            for (string::const_iterator it1 = m_hint.begin(), it2 = str.begin();
                 (it1 != m_hint.end()) && (it2 != str.end());
                 it1++, it2++) {
                if (*it1 != *it2)
                    break;
                commonSymbols++;
            }

            m_hint = m_hint.substr(0, commonSymbols);
        }
    }
}

const vector<reference_wrapper<const string> >& Complementer::getHints() const
{
    return m_hints;
}

const __cxx11::string& Complementer::getHint(const string& ifNotFound) const
{
    return m_hint.size() > 0 ? m_hint
                             : ifNotFound;
}
