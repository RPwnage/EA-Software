#! /bin/bash

echo "Retrieving the annotations from the node: ${MY_NODE_NAME}"

# MY_NODE_NAME set by spec.nodeName as an env var in the workload yaml.
# To support multiple annotations loop through and gather all desired ones from the current node.
echo '-DHOST_OVERRIDE='`./kubectl get node ${MY_NODE_NAME} -o jsonpath='{.metadata.annotations.cloud\.ea/dns-name}'` > /var/init-data/blaze.init
echo '-DINT_HOST_OVERRIDE='`./kubectl get node ${MY_NODE_NAME} -o jsonpath='{.metadata.annotations.cloud\.ea/internal-dns-name}'` >> /var/init-data/blaze.init

