set -e

BRANCH="$1"

restore-from-gh() {
    if [[ "$1" == "" ]]; then
        echo "ERROR, reverting back to ${BRANCH}"
    fi;
    rm -rf *
    git reset --hard HEAD
    git checkout ${BRANCH}
    git reset HEAD~
    if [[ "${STASH}" != 'No local changes to save' ]]; then
        git stash pop
    fi
    popd
}

make html
git add -f .
git commit -m "tmp"
STASH="$(git stash)"
pushd ..
git checkout gh-pages
git restore --source=${BRANCH} doc || restore-from-gh
cp -r doc/build/html/. .
find doc/build/html | while read fname; do
    [[ -f $fname ]] || continue;
    git add -f "$(echo $fname | cut -c 16-)" || restore-from-gh
done
COMMIT_MSG="$(python3 make_commit_msg.py ${BRANCH})" || restore-from-gh
git add -f .commits || restore-from-gh
git commit -m "${COMMIT_MSG}"
restore-from-gh 0
