import json

filepath = r'c:\Users\Yuta_Matsuo\siv3d\Re-buttlerX\projects\Game\App\assets\data\cards.json'

with open(filepath, 'r', encoding='utf-8-sig') as f:
    data = json.load(f)

# 属性をバランスよく割り当て: 1=量(赤), 2=質(青), 3=反撃(黄)
for i, card in enumerate(data['cards']):
    card['attribute'] = (i % 3) + 1

with open(filepath, 'w', encoding='utf-8-sig') as f:
    json.dump(data, f, ensure_ascii=False, indent=4)

print(f'カード {len(data["cards"])} 枚に属性を追加しました')
