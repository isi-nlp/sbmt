cmake_minimum_required(VERSION 2.8)

project(pipelines)

install(DIRECTORY corpus-pipeline 
                  decode-pipeline 
                  ruleset-pipeline
                  tune-pipeline
                  lib
                  scripts
                  util
        DESTINATION . 
        USE_SOURCE_PERMISSIONS
        PATTERN .* EXCLUDE
        PATTERN *~ EXCLUDE
        PATTERN *.pyc EXCLUDE
        )
file(GLOB TOPSH *.sh)
install(PROGRAMS ${TOPSH} DESTINATION .)
install(FILES README DESTINATION .)