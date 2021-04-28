import os
import argparse
import errno, os, stat, shutil
from distutils.dir_util import copy_tree
from git import Repo
from git import Git
from git import GitCommandError


def handleRemoveReadonly(func, path, exc):
  excvalue = exc[1]
  if func in (os.unlink, os.rmdir, os.remove) and excvalue.errno == errno.EACCES:
      os.chmod(path, stat.S_IRWXU| stat.S_IRWXG| stat.S_IRWXO) # 0777
      func(path)
  else:
    raise

def print_repository(repo):
    print('Repo description: {}'.format(repo.description))
    print('Repo active branch is {}'.format(repo.active_branch))
    for remote in repo.remotes:
        print('Remote named "{}" with URL "{}"'.format(remote, remote.url))
    print('Last commit for repo is {}.'.format(str(repo.head.commit.hexsha)))

def merge_branches(repo, source, target):
    sourceBranch = repo.branches[source]
    targetBranch = repo.branches[target]
    try:
        repo.git.checkout(source)
        repo.git.checkout(target)
        repo.git.pull()
        repo.git.merge(source)

        # try:
        #     repo.git.commit(m='Merge {} into {}'.format(source, target))
        # except GitCommandError:
        #     print("No merge needed.")
        # else:
        repo.git.push('--set-upstream', 'origin', target)
    except GitCommandError as exc:
            print(exc.stderr)
            exit(1)

    print("Merged {} to {}".format(source, target))


parser = argparse.ArgumentParser(description='Framework documentation deploy script.')
parser.add_argument('--docsPath', type=str, dest='docsPath', help='path of the main Framework documentation content')
parser.add_argument('--gitRepo', type=str, dest='gitRepo', help='git repo to clone and push changes to')
parser.add_argument('--gitBranch', type=str, dest='gitBranch', help='git branch to clone and push changes to', default='master')
parser.add_argument('--tempFolder', type=str, dest='tempFolder', help='temporary folder to use')
parser.add_argument('--sshIdFile', type=str, dest='sshIdFile', help='SSH id for authentication')
parser.add_argument('--gitBinary', type=str, dest='gitBinary', help='Git Binary to use')
args = parser.parse_args()
print(args)


# 1. Clone the gitlab repo to a temporary location
repoPath = args.tempFolder
# Empty the folder in case there are leftovers
try:
    shutil.rmtree(repoPath, ignore_errors=False, onerror=handleRemoveReadonly)
except FileNotFoundError:
    pass

envDict = {
    "GIT_SSH_COMMAND": 'ssh -i %s' % args.sshIdFile.replace("\\","/")
    }

if (args.gitBinary):
    envDict['GIT_PYTHON_GIT_EXECUTABLE'] = args.gitBinary
print(envDict)

cloned_repo = Repo.clone_from(url = args.gitRepo, to_path = repoPath, env = envDict)
assert cloned_repo.__class__ is Repo     # clone an existing repository
assert Repo.init(repoPath).__class__ is Repo

cloned_repo.git.checkout(args.gitBranch)

# print some repo info
print_repository(cloned_repo)

# 2. Copy the content md files
localPath = args.docsPath


# Remove the existing content so we also delete content
try:
    for entry in os.scandir(repoPath):
        if '.git' not in entry.name and entry.is_dir():
            shutil.rmtree(entry.path, ignore_errors=False, onerror=handleRemoveReadonly)
        elif entry.is_file():
            os.chmod(entry.path, stat.S_IRWXU| stat.S_IRWXG| stat.S_IRWXO) # 0777
            os.unlink(entry.path)

except FileNotFoundError:
    pass

copy_tree(localPath, repoPath, preserve_mode=0)

# 4. git add .
# 5. git commit
# 6. git push
cloned_repo.git.status()
cloned_repo.git.add(os.path.join(repoPath, '.'))
cloned_repo.git.status()
try:
    cloned_repo.git.commit(m='Updating documentation')
except GitCommandError:
    print("Files up to date.")
else:
    cloned_repo.git.push('--set-upstream', 'origin', args.gitBranch)
    print("Files committed to branch {}.".format(args.gitBranch))

# Merge dev-na-cm with master (could have changes)
if args.gitBranch != "master":
    merge_branches(cloned_repo, "master", args.gitBranch)
    merge_branches(cloned_repo, args.gitBranch, "master")
    cloned_repo.git.checkout("master")
    cloned_repo.git.pull()

print("Updating local documentation with merged result")
# Remove the existing content so we also delete content
try:
    shutil.rmtree(localPath, ignore_errors=False, onerror=handleRemoveReadonly)
except FileNotFoundError:
    pass

copy_tree(repoPath, localPath, preserve_mode=0)

# We don't want to check in the .git repo datafiles, remove them.
gitFolder = os.path.join(localPath, '.git')
try:
    shutil.rmtree(gitFolder, ignore_errors=False, onerror=handleRemoveReadonly)
except FileNotFoundError:
    pass

exit(0)