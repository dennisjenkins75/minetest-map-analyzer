-- Contains schema of database that our tool WRITES to.

create table `actor` (
  actor_id integer primary key,
  name text not null
);

create table `node` (
  node_id integer primary key,
  name text not null
);

create table `nodes` (
  -- Where is the node (64-bit signed int)?
  pos_id integer primary key,

  -- For convenience (-32768 to +32767).
  node_x integer not null,
  node_y integer not null,
  node_z integer not null,

  -- Who owns it (meta.owner -> `actor` table), if known.
  -- owner_id 0 means "no owner" (must have entry in `actor` table).
  owner_id integer not null,

  -- What type it is (`node` table).
  node_id integer not null,

  -- Total value of all minegeld held in inventory.
  minegeld integer not null default 0,

  constraint owner_id_fk foreign key (owner_id) references actor (actor_id)
    on delete cascade
);

create table `inventory` (
  -- Which node holds this inventory.
  pos_id integer,

  -- Inventory type name ("src", "dest", "main")
  type text,

  -- Whatever is there.
  item_string text,

  constraint pos_id_fk foreign key (pos_id) references nodes (pos_id)
    on delete cascade
);

-- For recording mapblocks that are 'interesting'.
create table `blocks` (
  -- Which mapblock.
  mapblock_id integer primary key,

  -- For convenience (-2048 to +2047).
  mapblock_x integer not null,
  mapblock_y integer not null,
  mapblock_z integer not null,

  -- Content_id for node if block is 100% uniform, 0 otherwise.
  uniform integer not null,

  constraint uniform_id_fk foreign key (uniform) references node (node_id)
    on delete cascade
);
