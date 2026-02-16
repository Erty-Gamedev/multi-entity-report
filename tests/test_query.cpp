#include "doctest.h"
#include "mer.h"


static const Entity entity{
	{ "classname", "monster_gman" },
	{ "targetname", "argumentg" },
	{ "origin", "32 -64 128" },
	{ "angles", "45 0 0" },
	{ "renderamt", "255" },
	{ "sc_mm_value_hash", "0.1#1" },
	{ "spawnflags", "19"}  // WaitTillSeen 1 | Gag 2 | Prisoner 16 = 19
};



TEST_SUITE("query parse")
{
	TEST_CASE("parse keyvalue")
	{
		Query query{ "targetname=test" };

		CHECK(query.key == "targetname");
		CHECK(query.value == "test");
		CHECK(query.op == Query::QueryEquals);
	}

	TEST_CASE("parse not equals keyvalue")
	{
		Query query{ "targetname!=test" };

		CHECK(query.key == "targetname");
		CHECK(query.value == "test");
		CHECK(query.op == Query::QueryNotEquals);
	}
	
	TEST_CASE("parse only key")
	{
		Query query{ "targetname=" };

		CHECK(query.key == "targetname");
		CHECK(query.value == "");
		CHECK(query.op == Query::QueryEquals);
	}
	
	TEST_CASE("parse only value")
	{
		Query query{ "=test" };

		CHECK(query.key == "");
		CHECK(query.value == "test");
		CHECK(query.op == Query::QueryEquals);
	}
	
	TEST_CASE("parse greater or equals to")
	{
		Query query{ "health>=50" };

		CHECK(query.key == "health");
		CHECK(query.value == "50");
		CHECK(query.op == Query::QueryGreaterEquals);
	}

	TEST_CASE("parse key with index")
	{
		Query query{ "origin[1]=100" };

		CHECK(query.key == "origin");
		CHECK(query.value == "100");
		CHECK(query.valueIndex == 1);
		CHECK(query.op == Query::QueryEquals);
	}

	TEST_CASE("parse mod")
	{
		Query query{ "valve" };
		CHECK(query.valid == false);
		CHECK(query.key == "valve");
	}
}

TEST_SUITE("query match")
{
	TEST_CASE("match equals")
	{
		SUBCASE("match classname")
		{
			Query query{ "classname=monster" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=monster_gman");
		}

		SUBCASE("match key")
		{
			Query query{ "class=" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=");
		}

		SUBCASE("match value")
		{
			Query query{ "=argument" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "targetname=argumentg");
		}

		SUBCASE("fail key")
		{
			Query query{ "damage=" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("fail value")
		{
			Query query{ "classname=weapon" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("match element")
		{
			Query query{ "origin[2]=128" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[2]=128");
		}

		SUBCASE("do not match element")
		{
			Query query{ "origin[1]!=0" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[1]!=0 (32 -64 128)");
		}

		SUBCASE("match missing element")
		{
			Query query{ "renderamt[1]=" + Query::c_empty };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt[1]=\"\"");
		}

		SUBCASE("non-empty element do not match empty value")
		{
			Query query{ "origin[2]=" + Query::c_empty };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("zero element do not match empty value")
		{
			Query query{ "angles[1]=" + Query::c_empty };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("match non-empty element")
		{
			Query query{ "origin[2]!=" + Query::c_empty };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[2]!=\"\"");
		}

		SUBCASE("do not match classname")
		{
			Query query{ "classname!=weapon" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname!=weapon (monster_gman)");
		}

		SUBCASE("do not match classname fail")
		{
			Query query{ "classname!=monster" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}
	}

	TEST_CASE("match exact equals")
	{
		SUBCASE("match classname")
		{
			Query query{ "classname==monster_gman" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=monster_gman");
		}

		SUBCASE("match key")
		{
			Query query{ "classname==" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=");
		}

		SUBCASE("match value")
		{
			Query query{ "==argumentg" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "targetname=argumentg");
		}

		SUBCASE("fail key")
		{
			Query query{ "class==monster_gman" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("fail value")
		{
			Query query{ "targetname==argument" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}
	}

	TEST_CASE("match greater than")
	{
		SUBCASE("match value greater than")
		{
			Query query{ "renderamt>5" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}

		SUBCASE("match element greater than")
		{
			Query query{ "origin[2]>16" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[2]=128");
		}

		SUBCASE("match empty is greater than negative")
		{
			Query query{ "renderamt[2]>-5" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt[2]>-5");
		}

		SUBCASE("match any value greater than")
		{
			Query query{ ">96" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}
	}

	TEST_CASE("match less than")
	{
		SUBCASE("match value less than")
		{
			Query query{ "origin<64" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin=32 -64 128");
		}

		SUBCASE("match element less than")
		{
			Query query{ "origin[1]<32" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[1]=-64");
		}

		SUBCASE("match empty is less than 5")
		{
			Query query{ "renderamt[1]<5" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt[1]<5");
		}

		SUBCASE("match any value less than")
		{
			Query query{ "<1" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "sc_mm_value_hash=0.1#1");
		}
	}

	TEST_CASE("match greater or equal than")
	{
		SUBCASE("match value greater or equal than")
		{
			Query query{ "renderamt>=255" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}

		SUBCASE("match element greater or equal than")
		{
			Query query{ "origin[2]>=16" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[2]=128");
		}

		SUBCASE("match empty is greater or equal to zero")
		{
			Query query{ "renderamt[2]>=0" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt[2]>=0");
		}

		SUBCASE("match any value greater or equal than")
		{
			Query query{ ">=96" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}
	}

	TEST_CASE("match less or equal than")
	{
		SUBCASE("match value less or equal than")
		{
			Query query{ "renderamt<=255" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}

		SUBCASE("match element less or equal than")
		{
			Query query{ "origin[1]<=16" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "origin[1]=-64");
		}

		SUBCASE("match empty is less or equal than 5")
		{
			Query query{ "renderamt[1]<=5" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt[1]<=5");
		}

		SUBCASE("match any value less or equal than")
		{
			Query query{ "<=1" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "sc_mm_value_hash=0.1#1");
		}
	}

	TEST_CASE("match spawnflags")
	{
		SUBCASE("match any flags")
		{
			Query query{ "spawnflags=2" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "spawnflags=19");
		}

		SUBCASE("match all flags")
		{
			Query query{ "spawnflags==3" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "spawnflags=19");
		}

		SUBCASE("fail all flags")
		{
			Query query{ "spawnflags==5" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}

		SUBCASE("match no flags")
		{
			Query query{ "spawnflags!=4" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "spawnflags!=4");
		}
	}
}

TEST_SUITE("query chain")
{
	TEST_CASE("chain AND")
	{
		SUBCASE("matched chain")
		{
			Query first{ "classname=monster" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=argument");
			first.type = Query::QueryAnd;
			first.next = second.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=monster_gman AND targetname=argumentg");
		}

		SUBCASE("matched chain long")
		{
			Query first{ "classname=monster" };
			first.type = Query::QueryAnd;
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=argument");
			second->type = Query::QueryAnd;
			std::unique_ptr<Query> third = std::make_unique<Query>("renderamt>0");
			third->type = Query::QueryAnd;
			std::unique_ptr<Query> fourth = std::make_unique<Query>("origin[1]<=16");
			fourth->type = Query::QueryAnd;
			first.next = second.get();
			second->next = third.get();
			third->next = fourth.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=monster_gman AND targetname=argumentg AND renderamt=255 AND origin[1]=-64");
		}

		SUBCASE("failed chain")
		{
			Query first{ "classname=monster" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=banana");
			first.type = Query::QueryAnd;
			first.next = second.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}
	}

	TEST_CASE("chain OR")
	{
		SUBCASE("matched chain")
		{
			Query first{ "classname=weapon_" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=argument");
			first.next = second.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "targetname=argumentg");
		}

		SUBCASE("matched chain long")
		{
			Query first{ "classname=weapon_" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=banana");
			std::unique_ptr<Query> third = std::make_unique<Query>("renderamt>0");
			std::unique_ptr<Query> fourth = std::make_unique<Query>("origin[1]>=16");
			first.next = second.get();
			second->next = third.get();
			third->next = fourth.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "renderamt=255");
		}

		SUBCASE("failed chain")
		{
			Query first{ "classname=monster" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=banana");
			first.type = Query::QueryAnd;
			first.next = second.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}
	}

	TEST_CASE("chain mixed")
	{
		SUBCASE("matched chain")
		{
			Query first{ "classname=weapon_" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=argument");
			second->type = Query::QueryAnd;
			std::unique_ptr<Query> third = std::make_unique<Query>("renderamt>0");
			first.next = second.get();
			second->next = third.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "targetname=argumentg AND renderamt=255");
		}

		SUBCASE("matched chain long")
		{
			Query first{ "classname=monster" };
			first.type = Query::QueryAnd;
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=banana");
			std::unique_ptr<Query> third = std::make_unique<Query>("renderamt<0");
			std::unique_ptr<Query> fourth = std::make_unique<Query>("origin[1]<=16");
			first.next = second.get();
			second->next = third.get();
			third->next = fourth.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == true);
			CHECK(entry.queryMatches == "classname=monster_gman AND origin[1]=-64");
		}

		SUBCASE("failed chain")
		{
			Query first{ "classname=weapon_" };
			std::unique_ptr<Query> second = std::make_unique<Query>("targetname=banana");
			std::unique_ptr<Query> third = std::make_unique<Query>("renderamt>0");
			third->type = Query::QueryAnd;
			std::unique_ptr<Query> fourth = std::make_unique<Query>("origin[1]>=16");
			first.next = second.get();
			second->next = third.get();
			third->next = fourth.get();

			EntityEntry entry = first.testChain(entity);
			CHECK(entry.matched == false);
			CHECK(entry.queryMatches == "");
		}
	}
}
