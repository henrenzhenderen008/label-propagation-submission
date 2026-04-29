#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <string>

using namespace std;

struct Node {
    int id;
    string label;
    vector<int> neighbours;
};

vector<string> split_csv(const string &line) {
    vector<string> fields;
    string current;
    bool in_quote = false;
    for (char c : line) {
        if (c == '"') {
            in_quote = !in_quote;
        } else if (c == ',' && !in_quote) {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

string strip_quotes(const string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

vector<int> parse_neighbours(string s) {
    vector<int> res;
    if (!s.empty() && s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s.pop_back();
    
    stringstream ss(s);
    string part;
    while (getline(ss, part, ',')) {
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);
        if (!part.empty()) {
            res.push_back(stoi(part));
        }
    }
    return res;
}

string get_most_frequent(const vector<string> &labels) {
    unordered_map<string, int> cnt;
    for (const auto &l : labels) cnt[l]++;

    string best_label;
    int max_cnt = -1;
    for (const auto &p : cnt) {
        const string &lab = p.first;
        int count = p.second;
        if (count > max_cnt) {
            max_cnt = count;
            best_label = lab;
        } else if (count == max_cnt && lab < best_label) {
            best_label = lab;
        }
    }
    return best_label;
}

int main(int argc, char *argv[]) {
    system("chcp 65001 > nul");
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "=== 程序启动 ===" << endl;

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.csv>" << endl;
        return 1;
    }
    cout << "尝试打开文件: " << argv[1] << endl;
    
    ifstream fin(argv[1]);
    if (!fin) {
        cerr << "【错误】打不开文件: " << argv[1] << endl;
        return 1;
    }
    cout << "【成功】文件已打开" << endl;

    // 【关键修复】用 unordered_map 直接存储节点，不使用指针
    unordered_map<int, Node> id2node;
    vector<int> all_ids; // 保存所有节点 ID 用于迭代

    string line;
    getline(fin, line); // 跳过表头
    cout << "跳过表头: " << line << endl;

    int line_num = 1;
    while (getline(fin, line)) {
        line_num++;
        auto fields = split_csv(line);
        if (fields.size() < 3) continue;
        
        int id = stoi(fields[0]);
        string label = strip_quotes(fields[1]);
        string neigh_str = strip_quotes(fields[2]);
        auto neighbours = parse_neighbours(neigh_str);

        // 直接存入 map
        id2node[id] = {id, label, neighbours};
        all_ids.push_back(id);
    }

    cout << endl << "=== 成功读取了 " << all_ids.size() << " 个节点 ===" << endl;
    if (all_ids.empty()) {
        cerr << "【错误】没有读取到任何节点！" << endl;
        return 1;
    }

    // 同步标签传播
    cout << endl << "=== 开始标签传播 ===" << endl;
    bool updated;
    int iter = 0;
    do {
        updated = false;
        unordered_map<int, string> new_labels;

        // 计算所有新标签
        for (int id : all_ids) {
            Node &node = id2node[id];
            vector<string> neighbour_labels;
            
            // 收集邻居标签（包括自己）
            for (int neigh_id : node.neighbours) {
                if (id2node.count(neigh_id)) {
                    neighbour_labels.push_back(id2node[neigh_id].label);
                }
            }
            new_labels[id] = get_most_frequent(neighbour_labels);
        }

        // 同步更新
        for (int id : all_ids) {
            if (id2node[id].label != new_labels[id]) {
                cout << "节点 " << id << " 从 " << id2node[id].label << " 变为 " << new_labels[id] << endl;
                id2node[id].label = new_labels[id];
                updated = true;
            }
        }
        iter++;
        cout << "迭代 " << iter << " 次完成" << endl;
    } while (updated);

    cout << endl << "=== 算法收敛，共迭代 " << iter << " 次 ===" << endl;

    // 按 ID 升序排序
    sort(all_ids.begin(), all_ids.end());

    // 输出 output.csv
    cout << endl << "正在生成 output.csv..." << endl;
    ofstream fout("output.csv");
    if (!fout) {
        cerr << "【错误】无法创建 output.csv" << endl;
        return 1;
    }
    fout << "id,label\n";
    for (int id : all_ids) {
        fout << id << "," << id2node[id].label << "\n";
    }
    fout.close();

    cout << "【成功】已生成 output.csv！" << endl;

    return 0;
}