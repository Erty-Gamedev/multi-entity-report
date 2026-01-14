#include "doctest.h"
#include "mer.h"


TEST_SUITE("query parse")
{
	TEST_CASE("parse keyvalue")
	{
		Query query{ "targetname=test" };

		CHECK(query.key == "targetname");
		CHECK(query.value == "test");
		CHECK(query.op == Query::QueryEquals);
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
		Entity entity{ { "classname", "monster_gman" } };

		SUBCASE("match classname")
		{
			Query query{ "classname=monster" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.key == "classname");
			CHECK(entry.value == "monster_gman");
		}

		SUBCASE("match key")
		{
			Query query{ "class=" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.key == "classname");
			CHECK(entry.value == "");
		}

		SUBCASE("match value")
		{
			Query query{ "=monster" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == true);
			CHECK(entry.key == "");
			CHECK(entry.value == "monster_gman");
		}

		SUBCASE("fail value")
		{
			Query query{ "classname=weapon" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.key == "classname");
			CHECK(entry.value == "");
		}

		SUBCASE("fail key")
		{
			Query query{ "targetname=" };
			EntityEntry entry = query.testEntity(entity);
			CHECK(entry.matched == false);
			CHECK(entry.key == "");
			CHECK(entry.value == "");
		}

	}
}
