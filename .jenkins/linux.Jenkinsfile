// Linux smoke test

// Load generators
node('master') {
  checkout scm
  generators = load '.jenkins/jobgenerators.groovy'
}

// Run Linux tests for Ubuntu 18.04
JOBS = [:]
JOBS['static'] = generators.buildLinuxJob('static', 'ubuntu1804-gcc', false)
JOBS['shared'] = generators.buildLinuxJob('shared', 'ubuntu1804-gcc', true)
parallel JOBS
