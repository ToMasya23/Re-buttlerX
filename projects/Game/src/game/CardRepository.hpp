# pragma once
# include "CardSpec.hpp"

namespace Cards
{
    inline Array<CardSpec> loadFromJSON()
    {
        const Array<FilePath> candidates = {
            U"assets/data/cards.json",
            U"App/assets/data/cards.json",
            U"../App/assets/data/cards.json",
            U"../../App/assets/data/cards.json",
        };
        JSON json;
        for (const auto& p : candidates)
        {
            if (!FileSystem::Exists(p))
            {
                continue;
            }
            TextReader tr{ p };
            if (!tr)
            {
                continue;
            }
            const String s = tr.readAll();
            json = JSON::Parse(s);
            if (json)
            {
                break;
            }
        }

        if (not json || !json.isObject())
        {
            return {
                CardSpec{ U"default_1", U"攻撃1", 10, 0.0, 1.0 },
                CardSpec{ U"default_2", U"攻撃2", 10, 0.0, 1.0 },
                CardSpec{ U"default_3", U"攻撃3", 10, 0.0, 1.0 },
                CardSpec{ U"default_4", U"攻撃4", 10, 0.0, 1.0 },
            };
        }

        const JSON cardsNode = json[U"cards"];
        if (!cardsNode || !cardsNode.isArray())
        {
            return {
                CardSpec{ U"default_1", U"攻撃1", 10, 0.0, 1.0 },
                CardSpec{ U"default_2", U"攻撃2", 10, 0.0, 1.0 },
                CardSpec{ U"default_3", U"攻撃3", 10, 0.0, 1.0 },
                CardSpec{ U"default_4", U"攻撃4", 10, 0.0, 1.0 },
            };
        }

        Array<CardSpec> loaded;
        for (const auto& jc : cardsNode.arrayView())
        {
            if (!jc.isObject())
            {
                continue;
            }
            CardSpec s;
            if (jc[U"id"].isString()) s.id = jc[U"id"].getString();
            if (jc[U"name"].isString()) s.name = jc[U"name"].getString();
            if (jc[U"cost"].isNumber()) s.cost = jc[U"cost"].get<int32>();
            if (jc[U"delay"].isNumber()) s.delaySec = static_cast<double>(jc[U"delay"].get<int32>());
            if (jc[U"weight"].isNumber()) s.weight = jc[U"weight"].get<double>();
            if (jc[U"attribute"].isNumber()) s.attributeId = jc[U"attribute"].get<int32>();
            if (jc[U"effects"].isArray())
            {
                for (const auto& effect : jc[U"effects"].arrayView())
                {
                    if (!effect.isObject()) continue;
                    if (!effect[U"type"].isString()) continue;
                    const String type = effect[U"type"].getString();
                    if (!effect[U"value"].isNumber()) continue;
                    if (type == U"damageHP")
                        s.damageHP = effect[U"value"].get<int32>();
                    else if (type == U"addCrazy")
                        s.addCrazy = effect[U"value"].get<int32>();
                }
            }
            loaded << s;
        }
        if (loaded.isEmpty())
        {
            loaded << CardSpec{ U"fallback", U"攻撃", 10, 0.0, 1.0 };
        }
        return loaded;
    }
}


