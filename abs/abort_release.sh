#! /bin/sh

git checkout ${COM_SA_DEV}/abs/APR
git checkout ${COM_SA_DEV}/deployment_templates/U1/package_instruction_comsa.txt
git checkout ${COM_SA_DEV}/abs/meta-comsa/recipes-comsa/com-sa/com-sa.bb
rm -f ${COM_SA_DEV}/abs/finalize_release.sh
