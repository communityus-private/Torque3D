<?php /* Smarty version 2.6.18, created on 2015-06-23 11:06:04
         compiled from vc2013_sln_x64.tpl */ ?>
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 2013
<?php $_from = $this->_tpl_vars['projects']; if (!is_array($_from) && !is_object($_from)) { settype($_from, 'array'); }$this->_foreach['projects'] = array('total' => count($_from), 'iteration' => 0);
if ($this->_foreach['projects']['total'] > 0):
    foreach ($_from as $this->_tpl_vars['project']):
        $this->_foreach['projects']['iteration']++;
?>
<?php if ($this->_tpl_vars['project']->projectFileExt == ".csproj"): ?>
Project("<?php echo '{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}'; ?>
") = "<?php echo $this->_tpl_vars['project']->name; ?>
", "projects/<?php echo $this->_tpl_vars['project']->name; ?>
<?php echo $this->_tpl_vars['project_ext']; ?>
", "<?php echo $this->_tpl_vars['project']->guid; ?>
"
<?php else: ?>
Project("<?php echo $this->_tpl_vars['solution_guid']; ?>
") = "<?php echo $this->_tpl_vars['project']->name; ?>
", "projects/<?php echo $this->_tpl_vars['project']->name; ?>
<?php echo $this->_tpl_vars['project_ext']; ?>
", "<?php echo $this->_tpl_vars['project']->guid; ?>
"
<?php endif; ?>
EndProject
<?php endforeach; endif; unset($_from); ?>
<?php $_from = $this->_tpl_vars['projectExtRefs']; if (!is_array($_from) && !is_object($_from)) { settype($_from, 'array'); }if (count($_from)):
    foreach ($_from as $this->_tpl_vars['pname'] => $this->_tpl_vars['v']):
?>
Project("<?php echo $this->_tpl_vars['v'][2]; ?>
") = "<?php echo $this->_tpl_vars['pname']; ?>
", "<?php echo $this->_tpl_vars['v'][0]; ?>
", "<?php echo $this->_tpl_vars['v'][1]; ?>
"
EndProject
<?php endforeach; endif; unset($_from); ?>
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|Win64 = Debug|Win64
		Optimized Debug|Win64 = Optimized Debug|Win64
		Release|Win64 = Release|Win64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
<?php $_from = $this->_tpl_vars['projects']; if (!is_array($_from) && !is_object($_from)) { settype($_from, 'array'); }$this->_foreach['projects'] = array('total' => count($_from), 'iteration' => 0);
if ($this->_foreach['projects']['total'] > 0):
    foreach ($_from as $this->_tpl_vars['project']):
        $this->_foreach['projects']['iteration']++;
?>
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Debug|Win64.ActiveCfg = Debug|x64
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Debug|Win64.Build.0 = Debug|x64
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Optimized Debug|Win64.ActiveCfg = Optimized Debug|x64
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Optimized Debug|Win64.Build.0 = Optimized Debug|x64
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Release|Win64.ActiveCfg = Release|x64
		<?php echo $this->_tpl_vars['project']->guid; ?>
.Release|Win64.Build.0 = Release|x64
<?php endforeach; endif; unset($_from); ?>
<?php $_from = $this->_tpl_vars['projectExtRefs']; if (!is_array($_from) && !is_object($_from)) { settype($_from, 'array'); }if (count($_from)):
    foreach ($_from as $this->_tpl_vars['pname'] => $this->_tpl_vars['v']):
?>
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Debug|Win64.ActiveCfg = Debug|x64
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Debug|Win64.Build.0 = Debug|x64
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Optimized Debug|Win64.ActiveCfg = Optimized Debug|x64
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Optimized Debug|Win64.Build.0 = Optimized Debug|x64
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Release|Win64.ActiveCfg = Release|x64
		<?php echo $this->_tpl_vars['v'][1]; ?>
.Release|Win64.Build.0 = Release|x64
<?php endforeach; endif; unset($_from); ?>
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal