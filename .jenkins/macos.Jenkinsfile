// macOS smoke test

// Load generators
node('master') {
  checkout scm
  generators = load '.jenkins/jobgenerators.groovy'
}

// Run macOS Tests
JOBS = [:]
JOBS['static'] = generators.buildMacOSJob('static', false)
JOBS['shared'] = generators.buildMacOSJob('shared', true)
parallel JOBS
