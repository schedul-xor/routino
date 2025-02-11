TILE0_HOST=tile0.ogiqvo.com
TILE1_HOST=tile1.ogiqvo.com

echo Copy to ${TILE0_HOST}
scp \
  src/router \
  src/router-slim \
  src/find_nearest_node \
  src/find_nearest_node-slim \
  src/planetsplitter \
  src/planetsplitter-slim \
  xml/tagging-drive.xml \
  xml/tagging-walk.xml \
  xor@${TILE0_HOST}:~/routino/tmp
scp web/bin/*  xor@${TILE0_HOST}:~/Documents/routino/web/bin
scp xml/routino-custom-profiles.xml xml/routino-translations.xml xor@${TILE0_HOST}:~/Documents/routino/xml/

echo Copy to ${TILE1_HOST}
scp \
  src/router \
  src/router-slim \
  src/find_nearest_node \
  src/find_nearest_node-slim \
  src/planetsplitter \
  src/planetsplitter-slim \
  xml/tagging-drive.xml \
  xml/tagging-walk.xml \
  xor@${TILE1_HOST}:~/routino/tmp
scp web/bin/*  xor@${TILE1_HOST}:~/Documents/routino/web/bin
scp xml/routino-custom-profiles.xml xml/routino-translations.xml xor@${TILE1_HOST}:~/Documents/routino/xml/
