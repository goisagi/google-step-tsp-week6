import sys
import random
import math

def read_input(filename):
    with open(filename) as f:
        cities = []
        for line in f.readlines()[1:]: # Ignore the first line.
            xy = line.split(',')
            cities.append((float(xy[0]), float(xy[1])))
        return cities

#全都市の距離表を作る
def build_distance_table(cities):
    n = len(cities)
    distance_table = [[0.0] * n for _ in range(n)]

    for i in range(n):
        x1, y1 = cities[i]
        for j in range(i + 1, n):
            x2, y2 = cities[j]
            dist = math.hypot(x1 - x2, y1 - y2)
            distance_table[i][j] = dist
            distance_table[j][i] = dist

    return distance_table

#巡回路の総距離を返す
def route_length(distance_table, route):
    total = 0.0
    for i in range(len(route)):
        total += distance_table[route[i]][route[(i + 1) % len(route)]]
    return total

# 貪欲法
def greedy_algorithm(distance_table, indices):
    if not indices:
        return []
    
    current = indices[0]
    route = [current]

    unvisited = set(indices)
    unvisited.remove(current)

    while unvisited:
        min_dist = float('inf')
        min_point = None

        for i in unvisited:
            dist = distance_table[current][i]
            if dist < min_dist:
                min_dist = dist
                min_point = i
        
        route.append(min_point)
        unvisited.remove(min_point)
        current = min_point
    return route

# 2-opt
def two_opt(distance_table, route):
    route_len = len(route)
    improved = True #更新の有無

    while improved:
        improved = False

        for i in range(route_len - 1):
            for j in range(i + 2, route_len):
                a = route[i]
                b = route[i + 1]
                c = route[j]
                d = route[(j + 1) % route_len]

                if distance_table[a][c] + distance_table[b][d] < distance_table[a][b] + distance_table[c][d]:
                    route[i+1:j+1] = reversed(route[i+1:j+1])
                    improved = True
    return route

# Or-opt (連続した1〜3都市の塊を取り出して別の場所に移動する)
def or_opt(distance_table, route):
    route_len = len(route)
    improved = True
    
    while improved:
        improved = False
        for chain_len in [3, 2, 1]: #動かす塊の長さを3, 2, 1の順で試す
            if improved: 
                break

            #塊の始点iを決定する
            for i in range(route_len):
                # 末尾をまたぐ複雑な処理は避ける
                if i + chain_len > route_len:
                    continue
                    
                # 動かすブロック(seg)と、残りのルート(rem)に完全に分離する
                seg = route[i : i + chain_len]
                rem = route[:i] + route[i + chain_len:]
                
                # ブロックを抜いたことで繋がる新しい枝の距離変化（マイナスになる）
                prev_node = rem[i - 1]
                next_node = rem[i % len(rem)]
                
                remove_cost = distance_table[prev_node][next_node] - \
                            distance_table[prev_node][seg[0]] - \
                            distance_table[seg[-1]][next_node]
                            
                best_j = -1
                min_delta = 0
                
                # 残りのルートのどこに挿入するのが一番良いか探す
                for j in range(len(rem)):
                    new_prev = rem[j]
                    new_next = rem[(j + 1) % len(rem)]
                    
                    insert_cost = distance_table[new_prev][seg[0]] + \
                                distance_table[seg[-1]][new_next] - \
                                distance_table[new_prev][new_next]
                                
                    delta = remove_cost + insert_cost
                    
                    if delta < min_delta - 1e-6:
                        min_delta = delta
                        best_j = j
                        
                if best_j != -1:
                    # 最適な位置に挿入してルートを更新
                    route = rem[:best_j+1] + seg + rem[best_j+1:]
                    improved = True
                    break
    return route

# ダブルブリッジ法 (4本の辺に切ってそれを別の順番で繋ぎ直す)
def double_bridge_kick(route):
    route_len = len(route)
    if route_len < 8:
        return route[:]

    #切断箇所を3箇所選ぶ
    cuts = sorted(random.sample(range(1, route_len - 1), 3))
    p1, p2, p3 = cuts
    
    A = route[:p1]
    B = route[p1:p2]
    C = route[p2:p3]
    D = route[p3:]
    
    return A + D + C + B

# 焼きなまし法
def simulated_annealing(distance_table, route):
    route_len = len(route)

    current_route = route[:]
    current_length = route_length(distance_table, current_route)

    best_route = current_route[:]
    best_length = current_length

    T = 1000.0
    
    inner_loops = min(route_len, 300) 
    cooling_rate = 0.9995

    while T > 0.01:
        #1回でなく複数回探索するように変更
        for _ in range(inner_loops):
            #2-optの切断位置をランダムで選択する
            i = random.randint(0, route_len - 3)
            j = random.randint(i + 2, route_len - 1)

            if i == 0 and j == route_len - 1:
                continue

            a = current_route[i]
            b = current_route[i + 1]
            c = current_route[j]
            d = current_route[(j + 1) % route_len]

            delta = distance_table[a][c] + distance_table[b][d] - distance_table[a][b] - distance_table[c][d]
            
            accept = False
            #距離が改善されるならば必ず採用
            if delta < 0:
                accept = True
            #距離が悪化する場合は、その差・温度・ランダムな数字によって採用するかを決定する
            else:
                if random.random() < math.exp(-delta / T):
                    accept = True

            if accept:
                current_route[i+1:j+1] = reversed(current_route[i+1:j+1])
                current_length += delta

                if current_length < best_length:
                    best_length = current_length
                    best_route = current_route[:]
        
        T *= cooling_rate
        
    return best_route

# 領域結合
def merge_tours(distance_table, route1, route2):
    min_delta = float('inf')
    best_i, best_j = -1, -1
    best_reverse = False

    for i in range(len(route1)): #route1の辺を選ぶ
        a = route1[i]
        b = route1[(i+1)%len(route1)]
        for j in range(len(route2)): #route2の辺を選ぶ
            c = route2[j]
            d = route2[(j+1)%len(route2)]
            
            #a---bとc---dの連結の解除による総距離の変化量
            break_cost = distance_table[a][b] + distance_table[c][d]
            
            #a---cとb---dで繋ぐ
            delta1 = distance_table[a][c] + distance_table[b][d] - break_cost
            if delta1 < min_delta:
                min_delta = delta1; best_i, best_j = i, j; best_reverse = True
            
            #a---dとb---cで繋ぐ
            delta2 = distance_table[a][d] + distance_table[b][c] - break_cost
            if delta2 < min_delta:
                min_delta = delta2; best_i, best_j = i, j; best_reverse = False

    r2_shifted = route2[best_j+1:] + route2[:best_j+1]
    if best_reverse:
        r2_shifted = r2_shifted[::-1]
    
    return route1[:best_i+1] + r2_shifted + route1[best_i+1:]

# 領域分割法
def divide_and_conquer(distance_table, cities, indices):
    if len(indices) <= 150:
        route = greedy_algorithm(distance_table, indices)
        return two_opt(distance_table, route)

    #x方向とy方向の広がりを調べる
    xs = [cities[i][0] for i in indices]
    ys = [cities[i][1] for i in indices]

    # 横長なら縦に、縦長なら横に切る
    if max(xs) - min(xs) > max(ys) - min(ys):
        indices.sort(key=lambda i: cities[i][0])
    else:
        indices.sort(key=lambda i: cities[i][1])
    mid = len(indices) // 2
    route_left = divide_and_conquer(distance_table, cities, indices[:mid])
    route_right = divide_and_conquer(distance_table, cities, indices[mid:])
    
    return merge_tours(distance_table, route_left, route_right)

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 week5_hw.py input_csv output_csv")
        return

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    cities = read_input(input_file)
    distance_table = build_distance_table(cities)
    
    print("1/4: 領域分割法で初期ルートを生成中...")
    indices = list(range(len(cities)))
    route = divide_and_conquer(distance_table, cities, indices)
    print(f"  -> 初期距離: {route_length(distance_table, route):.2f}")
    
    print("2/4: 2-opt と Or-opt で最適化中...")
    route = two_opt(distance_table, route)
    route = or_opt(distance_table, route)
    print(f"  -> 局所探索後の距離: {route_length(distance_table, route):.2f}")
    
    print("3/4: 焼きなまし法 (SA) を実行中...")
    route = simulated_annealing(distance_table, route)
    print(f"  -> SA後の距離: {route_length(distance_table, route):.2f}")
    
    print("4/4: 反復局所探索 (ILS) を実行中...")
    best_length = route_length(distance_table, route)
    best_route = route[:]
    
    for k in range(5):
        kicked_route = double_bridge_kick(best_route)
        kicked_route = two_opt(distance_table, kicked_route)
        kicked_route = or_opt(distance_table, kicked_route)
        
        current_length = route_length(distance_table, kicked_route)
        if current_length < best_length:
            best_length = current_length
            best_route = kicked_route[:]
            print(f"  -> キック {k+1}回目: 改善しました ({best_length:.2f})")
            
    route = best_route
    
    output_str = 'index\n'+'\n'.join(map(str,route))
    with open(output_file, 'w') as f:
        f.write(output_str)
    
    print("---------------------------------")
    print(f"最終的な総距離: {route_length(distance_table, route):.2f}")

if __name__ == "__main__":
    main()