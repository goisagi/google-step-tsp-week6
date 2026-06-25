#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <set>
#include <unordered_set>
using namespace std;

// -------------------- RNG --------------------
random_device rd;
mt19937 rng(rd());

double rand01()
{
    return uniform_real_distribution<double>(0.0, 1.0)(rng);
}

int randint(int l, int r)
{
    return uniform_int_distribution<int>(l, r)(rng);
}

// -------------------- 基本 --------------------
typedef pair<double, double> City;

vector<vector<double>> build_distance_table(const vector<City> &cities)
{
    int n = cities.size();
    vector<vector<double>> distance_table(n, vector<double>(n, 0.0));

    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            double dx = cities[i].first - cities[j].first;
            double dy = cities[i].second - cities[j].second;
            double dist = hypot(dx, dy);
            distance_table[i][j] = dist;
            distance_table[j][i] = dist;
        }
    }

    return distance_table;
}

vector<City> read_input(const string &filename)
{
    ifstream f(filename);
    vector<City> cities;

    string line;
    getline(f, line); // skip header

    while (getline(f, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        string x, y;
        getline(ss, x, ',');
        getline(ss, y, ',');
        cities.push_back(make_pair(stod(x), stod(y)));
    }
    return cities;
}

double route_length(const vector<vector<double>> &distance_table, const vector<int> &route)
{
    double sum = 0;
    int n = route.size();
    for (int i = 0; i < n; i++)
    {
        sum += distance_table[route[i]][route[(i + 1) % n]];
    }
    return sum;
}

// -------------------- greedy --------------------
vector<int> greedy_algorithm(const vector<vector<double>> &distance_table, const vector<int> &indices)
{
    if (indices.empty())
        return vector<int>();

    vector<int> route;
    unordered_set<int> unvisited(indices.begin(), indices.end());

    int current = indices[0];
    route.push_back(current);
    unvisited.erase(current);

    while (!unvisited.empty())
    {
        double min_dist = 1e18;
        int min_point = -1;

        for (unordered_set<int>::iterator it = unvisited.begin(); it != unvisited.end(); ++it)
        {
            int i = *it;
            double dist = distance_table[current][i];
            if (dist < min_dist)
            {
                min_dist = dist;
                min_point = i;
            }
        }

        route.push_back(min_point);
        unvisited.erase(min_point);
        current = min_point;
    }
    return route;
}

// -------------------- 2-opt --------------------
vector<int> two_opt(const vector<vector<double>> &distance_table, vector<int> route)
{
    int n = route.size();
    bool improved = true;

    while (improved)
    {
        improved = false;

        for (int i = 0; i < n - 1; i++)
        {
            for (int j = i + 2; j < n; j++)
            {
                int a = route[i];
                int b = route[i + 1];
                int c = route[j];
                int d = route[(j + 1) % n];

                double old_cost = distance_table[a][b] + distance_table[c][d];
                double new_cost = distance_table[a][c] + distance_table[b][d];

                if (new_cost < old_cost)
                {
                    reverse(route.begin() + i + 1, route.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
    return route;
}

// -------------------- or-opt --------------------
vector<int> or_opt(const vector<vector<double>> &distance_table, vector<int> route)
{
    int n = route.size();
    bool improved = true;

    while (improved)
    {
        improved = false;

        int chain_lens[] = {3, 2, 1};
        for (int ci = 0; ci < 3; ci++)
        {
            int chain_len = chain_lens[ci];
            if (improved)
                break;

            for (int i = 0; i < n; i++)
            {
                if (i + chain_len > n)
                    continue;

                vector<int> seg(route.begin() + i, route.begin() + i + chain_len);
                vector<int> rem;
                for (int k = 0; k < n; k++)
                {
                    if (k < i || k >= i + chain_len)
                        rem.push_back(route[k]);
                }

                int rem_n = rem.size();
                if (rem_n == 0)
                    continue;

                int prev_node = rem[(i - 1 + rem_n) % rem_n];
                int next_node = rem[i % rem_n];

                double remove_cost =
                    distance_table[prev_node][next_node] - distance_table[prev_node][seg[0]] - distance_table[seg.back()][next_node];

                double min_delta = 0;
                int best_j = -1;

                for (int j = 0; j < rem_n; j++)
                {
                    int new_prev = rem[j];
                    int new_next = rem[(j + 1) % rem_n];

                    double insert_cost =
                        distance_table[new_prev][seg[0]] +
                        distance_table[seg.back()][new_next] -
                        distance_table[new_prev][new_next];

                    double delta = remove_cost + insert_cost;

                    if (delta < min_delta - 1e-6)
                    {
                        min_delta = delta;
                        best_j = j;
                    }
                }

                if (best_j != -1)
                {
                    vector<int> new_route;
                    for (int k = 0; k <= best_j; k++)
                        new_route.push_back(rem[k]);
                    new_route.insert(new_route.end(), seg.begin(), seg.end());
                    for (int k = best_j + 1; k < rem_n; k++)
                        new_route.push_back(rem[k]);

                    route = new_route;
                    improved = true;
                    break;
                }
            }
        }
    }
    return route;
}

// -------------------- double bridge --------------------
vector<int> double_bridge_kick(vector<int> route)
{
    int n = route.size();
    if (n < 8)
        return route;

    vector<int> cuts;
    set<int> used;
    while ((int)cuts.size() < 3)
    {
        int v = randint(1, n - 2);
        if (!used.count(v))
        {
            used.insert(v);
            cuts.push_back(v);
        }
    }
    sort(cuts.begin(), cuts.end());

    int p1 = cuts[0], p2 = cuts[1], p3 = cuts[2];

    vector<int> A(route.begin(), route.begin() + p1);
    vector<int> B(route.begin() + p1, route.begin() + p2);
    vector<int> C(route.begin() + p2, route.begin() + p3);
    vector<int> D(route.begin() + p3, route.end());

    vector<int> res;
    res.insert(res.end(), A.begin(), A.end());
    res.insert(res.end(), D.begin(), D.end());
    res.insert(res.end(), C.begin(), C.end());
    res.insert(res.end(), B.begin(), B.end());

    return res;
}

// -------------------- SA --------------------
vector<int> simulated_annealing(const vector<vector<double>> &distance_table, vector<int> route)
{
    int n = route.size();

    vector<int> best_route = route;
    double best_length = route_length(distance_table, route);

    double T = 1000.0;
    int inner_loops = min(n, 300);
    double cooling_rate = 0.9995;

    double current_length = best_length;

    while (T > 0.01)
    {
        for (int _ = 0; _ < inner_loops; _++)
        {
            int i = randint(0, n - 3);
            int j = randint(i + 2, n - 1);

            if (i == 0 && j == n - 1)
                continue;

            int a = route[i];
            int b = route[i + 1];
            int c = route[j];
            int d = route[(j + 1) % n];

            double delta =
                distance_table[a][c] + distance_table[b][d] - distance_table[a][b] - distance_table[c][d];

            bool accept = false;

            if (delta < 0)
            {
                accept = true;
            }
            else if (rand01() < exp(-delta / T))
            {
                accept = true;
            }

            if (accept)
            {
                reverse(route.begin() + i + 1, route.begin() + j + 1);
                current_length += delta;

                if (current_length < best_length)
                {
                    best_length = current_length;
                    best_route = route;
                }
            }
        }
        T *= cooling_rate;
    }

    return best_route;
}

// -------------------- merge --------------------
vector<int> merge_tours(const vector<vector<double>> &distance_table,
                        const vector<int> &r1,
                        const vector<int> &r2)
{

    double min_delta = 1e18;
    int best_i = -1, best_j = -1;
    bool best_reverse = false;

    int n1 = r1.size();
    int n2 = r2.size();

    for (int i = 0; i < n1; i++)
    {
        int a = r1[i];
        int b = r1[(i + 1) % n1];

        for (int j = 0; j < n2; j++)
        {
            int c = r2[j];
            int d = r2[(j + 1) % n2];

            double break_cost = distance_table[a][b] + distance_table[c][d];

            double delta1 =
                distance_table[a][c] + distance_table[b][d] - break_cost;

            if (delta1 < min_delta)
            {
                min_delta = delta1;
                best_i = i;
                best_j = j;
                best_reverse = true;
            }

            double delta2 =
                distance_table[a][d] + distance_table[b][c] - break_cost;

            if (delta2 < min_delta)
            {
                min_delta = delta2;
                best_i = i;
                best_j = j;
                best_reverse = false;
            }
        }
    }

    vector<int> r2_shifted;
    for (int i = best_j + 1; i < n2; i++)
        r2_shifted.push_back(r2[i]);
    for (int i = 0; i <= best_j; i++)
        r2_shifted.push_back(r2[i]);

    if (best_reverse)
        reverse(r2_shifted.begin(), r2_shifted.end());

    vector<int> res;
    for (int i = 0; i <= best_i; i++)
        res.push_back(r1[i]);
    for (int ri = 0; ri < (int)r2_shifted.size(); ri++)
        res.push_back(r2_shifted[ri]);
    for (int i = best_i + 1; i < n1; i++)
        res.push_back(r1[i]);

    return res;
}

// -------------------- divide & conquer --------------------
struct CompareByX
{
    const vector<City> &cities;
    CompareByX(const vector<City> &c) : cities(c) {}
    bool operator()(int a, int b) const
    {
        return cities[a].first < cities[b].first;
    }
};

struct CompareByY
{
    const vector<City> &cities;
    CompareByY(const vector<City> &c) : cities(c) {}
    bool operator()(int a, int b) const
    {
        return cities[a].second < cities[b].second;
    }
};

vector<int> divide_and_conquer(const vector<vector<double>> &distance_table, const vector<City> &cities, vector<int> indices)
{
    if (indices.size() <= 150)
    {
        vector<int> route = greedy_algorithm(distance_table, indices);
        return two_opt(distance_table, route);
    }

    vector<double> xs, ys;
    for (int ii = 0; ii < (int)indices.size(); ii++)
    {
        int i = indices[ii];
        xs.push_back(cities[i].first);
        ys.push_back(cities[i].second);
    }

    double dx = *max_element(xs.begin(), xs.end()) - *min_element(xs.begin(), xs.end());
    double dy = *max_element(ys.begin(), ys.end()) - *min_element(ys.begin(), ys.end());

    if (dx > dy)
    {
        sort(indices.begin(), indices.end(), CompareByX(cities));
    }
    else
    {
        sort(indices.begin(), indices.end(), CompareByY(cities));
    }

    int mid = indices.size() / 2;

    vector<int> left(indices.begin(), indices.begin() + mid);
    vector<int> right(indices.begin() + mid, indices.end());

    vector<int> L = divide_and_conquer(distance_table, cities, left);
    vector<int> R = divide_and_conquer(distance_table, cities, right);

    return merge_tours(distance_table, L, R);
}

// -------------------- main --------------------
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cout << "Usage: ./a.out input_csv output_csv\n";
        return 0;
    }

    string input_file = argv[1];
    string output_file = argv[2];

    vector<City> cities = read_input(input_file);
    vector<vector<double>> distance_table = build_distance_table(cities);

    cout << "1/4: 領域分割法で初期ルートを生成中...\\n";
    vector<int> indices(cities.size());
    iota(indices.begin(), indices.end(), 0);

    vector<int> route = divide_and_conquer(distance_table, cities, indices);
    cout << "  -> 初期距離: " << route_length(distance_table, route) << "\n";

    cout << "2/4: 2-opt と Or-opt で最適化中...\n";
    route = two_opt(distance_table, route);
    route = or_opt(distance_table, route);
    cout << "  -> 局所探索後の距離: " << route_length(distance_table, route) << "\n";

    cout << "3/4: 焼きなまし法 (SA) を実行中...\n";
    route = simulated_annealing(distance_table, route);
    cout << "  -> SA後の距離: " << route_length(distance_table, route) << "\n";

    cout << "4/4: 反復局所探索 (ILS) を実行中...\n";

    double best_length = route_length(distance_table, route);
    vector<int> best_route = route;

    for (int k = 0; k < 5; k++)
    {
        vector<int> kicked = double_bridge_kick(best_route);
        kicked = two_opt(distance_table, kicked);
        kicked = or_opt(distance_table, kicked);

        double cur = route_length(distance_table, kicked);

        if (cur < best_length)
        {
            best_length = cur;
            best_route = kicked;
            cout << "  -> キック " << k + 1 << "回目: 改善しました (" << best_length << ")\n";
        }
    }

    route = best_route;

    ofstream f(output_file);
    f << "index\n";
    for (int fi = 0; fi < (int)route.size(); fi++)
        f << route[fi] << "\\n";

    cout << "---------------------------------\n";
    cout << "最終的な総距離: " << route_length(distance_table, route) << "\n";

    return 0;
}