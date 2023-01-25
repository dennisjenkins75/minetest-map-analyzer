# Postgresql Database Support

Dumping ground for notes on how I tested Postgresql support.

Based on https://forum.minetest.net/viewtopic.php?t=16689

## Postgresql database setup

```
$ psql -Upostgres -l | cat
$ psql -Upostgres -dpostgres -c "create user minetest with password '12345';"
$ psql -Upostgres -dpostgres -c "create database mt_test_1 owner minetest;"
$ psql -Upostgres -dmt_test_1 \
  -c "grant all privileges on database mt_test_1 to minetest;"
```

Edit a `world.mt` file to include:

```
backend = sqlite3
pgsql_connection = host=127.0.0.1 port=5432 user=minetest password=12345 dbname=my_test_1
pgsql_auth_connection = host=127.0.0.1 port=5432 user=minetest password=12345 dbname=my_test_1
pgsql_player_connection = host=127.0.0.1 port=5432 user=minetest password=12345 dbname=my_test_1
```

Generate Postgresql map from an sqlite map:

```
$ minetestserver --migrate postgresql --world \
  "${HOME}/.minetest/worlds/mt_test_1"
$ minetestserver --migrate-auth postgresql --world \
  "${HOME}/.minetest/worlds/mt_test_1"
$ minetestserver --migrate-players postgresql --world \
  "${HOME}/.minetest/worlds/mt_test_1"
```

## Running the map analyzer

Note that the argument to the `--map` flag is a QUOTED STRING WITH SPACES.
It is passed to `PQconnectdb()` unmodified by `libpqxx`.  See
https://www.postgresql.org/docs/current/libpq-connect.html, section 34.1.2
"Parameter Key Words" for more details.

```
$ ./bin/Release/map_analyzer --driver postgresql \
  --map "user=minetest password=12345 dbname=mt_test_1" \
  --out output-demo.sqlite --threads 12

$ sqlite3 ./output-demo.sqlite < reports/nodes-by-type.sql
```
