# ADR 5: Git Branching Model

## Context

Big Ladder is exploring a formal branching and usage model for Git.
We wish to do this to enhance developer productivity opening chances for taking advantages of opportunities and reducing the consequences of mistakes.
We also wish to enhance the inter-developer collaboration process.

## Decision

We've been exploring the [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/) git branching and usage model.
A quick "cheat-sheet" for the Git Flow model is available at `docs/resources/Git-branching-model.pdf`.

## Status

Tentative, but switching to using it now.

## Consequences

There are more active branch tags and restrictions on where branches can start from and where they merge to.
Each branch has a specific purpose.

A quick summary is as follows:

**master branch**

Only holds tagged releases.

**develop branch**

The main branch with HEAD always reflecting the latest delivered development changes.

**feature branches**

- May branch off from: `develop` 
- Must merge back into: `develop` 
- Branch naming convention: anything except `master`, `develop`, `release-/*`, or `hotfix-/*`

For developing new features.

**release branches**

- May branch off from: develop
- Must merge back into: `develop` and `master`
- Branch naming convention: `release-\*`

For preparing new releases.
This branch would be for bug fixes and for updating version numbers, etc.

**hotfix branches**

- May branch off from: `master`
- Must merge back into: `develop` and `master`
- Branch naming convention: `hotfix-/*` 

For patching and bug fixing production or released versions.
