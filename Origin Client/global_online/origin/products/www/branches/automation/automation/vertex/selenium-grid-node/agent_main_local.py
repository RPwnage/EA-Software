#!/usr/bin/env python

import sys
sys.path.append('src')

from grid_node_rest_agent import app

##
# Entry point of Grid Node Agent service.
# Script development dev local should use this entry point.
#
if __name__ == "__main__":
    print('starting grid node agent ...')
    app.run()
    print('grid node agent stopped')