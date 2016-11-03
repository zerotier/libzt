#!/usr/bin/env groovy

def changelog = getChangeLog currentBuild
slackSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"

parallel 'centos7': {
    node('centos7') {
        try {
            checkout scm
            stage('Build centos7') {
                sh 'make linux_service_and_intercept SDK_LWIP=1 SDK_IPV4=1'
            }
            stage('Run Basic Unit Test') {
                sh 'make unit_test SDK_LWIP=1 SDK_IPV4=1'
                sh './tests/unit/docker/start/sh'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
    }
}, 'macOS': {
    node('macOS') {
        try {
            checkout scm
            stage('Build macOS') {
                sh 'make osx_service_and_intercept SDK_LWIP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
    }
}

slackSend color: "#00ff00", message: "${env.JOB_NAME} #${env.BUILD_NUMBER} Complete (<${env.BUILD_URL}|Show More...>)"
