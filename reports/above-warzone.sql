select
  uniform, anthropocene, preserve,
  (mapblock_x) * 16 as node_x,
  (mapblock_y) * 16 as node_y,
  (mapblock_z) * 16 as node_z,
  floor(mapblock_y * 16 / 1000) * 1000 as stratum,
  (min(mapblock_y) * 16) as min_y,
  (max(mapblock_y) * 16) as max_y
from blocks
where (mapblock_y > (18000 / 16)) and not uniform
group by 1,2,3,4,5,6
order by stratum desc, mapblock_id
limit 20
