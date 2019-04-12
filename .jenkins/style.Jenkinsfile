// Style test

// Load generators
node('master') {
  checkout scm
  generators = load '.jenkins/jobgenerators.groovy'
}

// Run style test
JOBS = [:]
JOBS["style"] = generators.buildStyleJob()
parallel JOBS
